#include "configuration.h"

#if NCTNL_FCU_MIRROR
#include "Channels.h"
#include "NctnlFcuLine.h"
#include "NctnlFcuMirror.h"
#include "NctnlTextStore.h"
#include "NodeDB.h"
#include "PowerStatus.h"
#include "SerialConsole.h"
#include "Throttle.h"
#include "concurrency/LockGuard.h"
#include "concurrency/OSThread.h"
#include "mesh-pb-constants.h"
#include <cstring>
#if !MESHTASTIC_EXCLUDE_GPS
#include "GPS.h"
#include "GPSUpdateScheduling.h"
#endif
#if HAS_SCREEN
#include "MessageStore.h"
#include "graphics/Screen.h"
#include "graphics/draw/NotificationRenderer.h"
#endif
#if !MESHTASTIC_EXCLUDE_NCTNL
#include "NctnlModule.h"
#endif

namespace NctnlFcuMirror
{
namespace
{
constexpr uint32_t HELLO_FAST_INTERVAL_MS = 15 * 1000;
constexpr uint32_t HELLO_FAST_UPTIME_S = 60;
constexpr uint32_t HELLO_INTERVAL_MS = 60 * 1000;
constexpr uint32_t BATTERY_INTERVAL_MS = 60 * 1000;
constexpr uint32_t POWER_POLL_MS = 1000;
constexpr uint32_t SYNC_GUARD_MS = 10 * 60 * 1000;
constexpr uint32_t SYNC_WAIT_MS = 30 * 1000;
constexpr uint32_t FCU_PRESENT_MS = 300 * 1000;
constexpr uint32_t FCU_GPS_INTERVAL_MS = 30 * 1000;
constexpr uint32_t GPS_LINE_FCU_INTERVAL_MS = 30 * 1000;
constexpr uint32_t GPS_LINE_INTERVAL_MS = 120 * 1000;
constexpr uint32_t GPS_REQUEST_TIMEOUT_MS = 30 * 1000;
constexpr uint32_t GPS_REPEAT_IGNORE_MS = 5 * 1000;
constexpr uint32_t REPLAY_POLL_MS = 20;
// Free TX space a replay line leaves for live lines (largest packet line is ~560 bytes).
constexpr size_t REPLAY_RESERVE = 640;
// After this long without room a replay line falls back to the normal drop rule.
constexpr uint32_t REPLAY_MAX_WAIT_MS = 2000;
// Worst-case hello (every name byte escaped as \u00XX) fits; see docs/nctnl-fcu-mirror-spec.md.
constexpr size_t JSON_MAX = 1024;
constexpr size_t PAYLOAD_MAX = meshtastic_MeshPacket_size > JSON_MAX ? meshtastic_MeshPacket_size : JSON_MAX;

enum class Emit { Written, Dropped, Suppressed, NoRoom };
enum class Replay : uint8_t { Idle, Chat, Data };

concurrency::Lock *mirrorLock;
concurrency::OSThread *mirrorThread;
uint32_t seq;
uint32_t dropped;
uint32_t lastHelloMs;
uint32_t lastBatteryMs;
uint8_t lastPowerFlags = 0xFF;
uint32_t lastUptimeMs;
uint64_t uptimeMs;
uint8_t encodeBuf[meshtastic_MeshPacket_size];
char jsonBuf[JSON_MAX];
char lineBuf[NctnlFcuLine::lineLen(PAYLOAD_MAX, NctnlFcuLine::MAX_EXTRA_LEN)];
meshtastic_MeshPacket packetCopy;

// API session and sync state; written from the SerialConsole thread, acted on in tick().
bool sessionActive;
bool bannerShowPending;
bool sessionEndPending;
bool sessionEndedByDisconnect;
bool helloPending;
bool syncWaiting;
uint32_t syncWaitStartMs;
bool syncEverAccepted;
uint32_t lastSyncAcceptMs;

Replay replay = Replay::Idle;
size_t replayIndex;
bool replayWaiting;
uint32_t replayWaitStartMs;

bool fcuPresent;
uint32_t lastFcuMs;

bool gpsRequested;
uint32_t gpsRequestMs;
uint32_t gpsRequestFixTime;
bool gpsCommandSeen;
uint32_t lastGpsCommandMs;
bool lastFix;
bool fixSeen;
uint32_t lastFixTime;
uint32_t lastFixSeenMs;
uint32_t lastGpsLineMs;

bool canEmit()
{
    return mirrorLock && isActive() && console && !console->isMirrorSuppressed();
}

void wake()
{
    if (mirrorThread)
        mirrorThread->setIntervalFromNow(0);
}

// Caller holds mirrorLock.
Emit writeLine(char type, const char *extra, const uint8_t *payload, size_t len, bool paced = false);

void writeError(const char *code, char forType, const char *reason)
{
    char text[48];
    const int n = snprintf(text, sizeof(text), "%c %s", forType, reason);
    if (n > 0 && (size_t)n < sizeof(text))
        writeLine('E', code, reinterpret_cast<const uint8_t *>(text), (size_t)n);
}

// Paced lines wait (NoRoom, no seq used) until the TX buffer has room for them plus REPLAY_RESERVE.
Emit writeLine(char type, const char *extra, const uint8_t *payload, size_t len, bool paced)
{
    const size_t n = NctnlFcuLine::formatLine(type, seq, extra, payload, len, lineBuf, sizeof(lineBuf));
    if (n == 0) {
        if (type != 'E')
            writeError("len", type, "line too long");
        return Emit::Dropped;
    }
    const size_t needed = n + NCTNL_FCU_TX_HEADROOM;
    const bool canWait = paced && needed < NCTNL_FCU_TX_BUFFER_SIZE;
    size_t reserve = 0;
    if (canWait)
        reserve = NCTNL_FCU_TX_BUFFER_SIZE - needed < REPLAY_RESERVE ? NCTNL_FCU_TX_BUFFER_SIZE - needed : REPLAY_RESERVE;
    switch (console->writeMirrorLine(reinterpret_cast<const uint8_t *>(lineBuf), n, reserve)) {
    case SerialConsole::MirrorWrite::Suppressed:
        return Emit::Suppressed;
    case SerialConsole::MirrorWrite::Dropped:
        if (canWait)
            return Emit::NoRoom;
        dropped++;
        seq++;
        return Emit::Dropped;
    case SerialConsole::MirrorWrite::Written:
        break;
    }
    seq++;
    return Emit::Written;
}

// Caller holds mirrorLock.
void writePacket(char type, const meshtastic_MeshPacket &p, const char *extra = nullptr)
{
    const size_t len = pb_encode_to_bytes(encodeBuf, sizeof(encodeBuf), &meshtastic_MeshPacket_msg, &p);
    if (len == 0)
        writeError("enc", type, "MeshPacket encode failed");
    else
        writeLine(type, extra, encodeBuf, len);
}

// Caller holds mirrorLock.
void writeJson(char type, const char *extra, size_t len, const char *what)
{
    if (len == 0)
        writeError("len", type, what);
    else
        writeLine(type, extra, reinterpret_cast<const uint8_t *>(jsonBuf), len);
}

bool isAdmin(const meshtastic_MeshPacket &p)
{
    return p.which_payload_variant == meshtastic_MeshPacket_decoded_tag && p.decoded.portnum == meshtastic_PortNum_ADMIN_APP;
}

// Caller holds mirrorLock.
void writeDecodedPacket(char type, const meshtastic_MeshPacket &p)
{
    if (!isAdmin(p)) {
        writePacket(type, p);
        return;
    }
    packetCopy = p;
    packetCopy.decoded.payload.size = 0;
    writePacket(type, packetCopy);
}

uint32_t uptimeSeconds()
{
    const uint32_t now = millis();
    uptimeMs += now - lastUptimeMs;
    lastUptimeMs = now;
    return (uint32_t)(uptimeMs / 1000);
}

const char *hardwareName()
{
    static const char full[] = xstr(HW_VENDOR);
    static const char prefix[] = "meshtastic_HardwareModel_";
    return strncmp(full, prefix, sizeof(prefix) - 1) == 0 ? full + sizeof(prefix) - 1 : full;
}

const char *buildLabel()
{
    const char *version = optstr(APP_VERSION);
    const char *shortVersion = optstr(APP_VERSION_SHORT);
    const size_t n = strlen(shortVersion);
    if (strncmp(version, shortVersion, n) == 0 && version[n] == '.' && version[n + 1])
        return version + n + 1;
    return optstr(APP_ENV);
}

bool gpsAvailable()
{
#if !MESHTASTIC_EXCLUDE_GPS
    return gps && gps->isConnected() && config.position.gps_mode == meshtastic_Config_PositionConfig_GpsMode_ENABLED;
#else
    return false;
#endif
}

void emitHello()
{
    const char *names[MAX_NUM_CHANNELS];
    for (size_t i = 0; i < MAX_NUM_CHANNELS; i++) {
        names[i] = "";
        if (i < channels.getNumChannels()) {
            const meshtastic_Channel &ch = channels.getByIndex(i);
            if (ch.role != meshtastic_Channel_Role_DISABLED && ch.has_settings)
                names[i] = ch.settings.name;
        }
    }

    concurrency::LockGuard guard(mirrorLock);
    if (!canEmit())
        return;
    NctnlFcuLine::Hello h = {};
    h.node = nodeDB->getNodeNum();
    h.hw = hardwareName();
    h.fw = optstr(APP_VERSION);
    h.nb = buildLabel();
    h.up = uptimeSeconds();
    h.boots = myNodeInfo.reboot_count;
    h.channels = names;
    h.channelCount = MAX_NUM_CHANNELS;
    h.drop = dropped;
    h.fcu = isActive();
    h.longName = owner.long_name;
    h.shortName = owner.short_name;
    h.gps = gpsAvailable();
    writeJson('H', nullptr, NctnlFcuLine::formatHelloJson(h, jsonBuf, sizeof(jsonBuf)), "hello JSON too long");
}

uint8_t powerFlags()
{
    return (powerStatus->getHasUSB() ? 1 : 0) | (powerStatus->getIsCharging() ? 2 : 0);
}

void emitBattery()
{
    int mv = -1;
    int pct = -1;
    if (powerStatus->isInitialized() && powerStatus->getHasBattery() && powerStatus->getBatteryVoltageMv() > 0) {
        mv = powerStatus->getBatteryVoltageMv();
        const int p = powerStatus->getBatteryChargePercent();
        pct = p <= 100 ? p : -1;
    }

    concurrency::LockGuard guard(mirrorLock);
    if (!canEmit())
        return;
    writeJson('B', nullptr,
              NctnlFcuLine::formatBatteryJson(mv, pct, powerStatus->getHasUSB(), powerStatus->getIsCharging(), jsonBuf,
                                              sizeof(jsonBuf)),
              "battery JSON too long");
}

void emitGpsLine(bool forceNoFix)
{
#if !MESHTASTIC_EXCLUDE_GPS
    lastGpsLineMs = millis();
    gpsRequested = false;
    const meshtastic_Position &p = gps->p;
    NctnlFcuLine::GpsState g = {};
    g.fix = gps->hasFix() && !forceNoFix;
    g.sats = p.sats_in_view;
    g.age = fixSeen ? (long)((millis() - lastFixSeenMs) / 1000) : -1;
    if (g.fix) {
        g.lat = p.latitude_i;
        g.lon = p.longitude_i;
        g.alt = p.altitude;
        g.hdop = p.HDOP;
        // GPS.cpp stores ground_speed in whole km/h.
        g.spd = (long)((p.ground_speed * 1000UL + 18) / 36);
        g.trk = p.ground_track;
        g.t = p.timestamp;
    }

    concurrency::LockGuard guard(mirrorLock);
    if (!canEmit())
        return;
    writeJson('G', nullptr, NctnlFcuLine::formatGpsJson(g, jsonBuf, sizeof(jsonBuf)), "GPS JSON too long");
#else
    (void)forceNoFix;
#endif
}

void emitGpsLineNow()
{
    emitGpsLine(false);
}

void updateGps()
{
#if !MESHTASTIC_EXCLUDE_GPS
    if (!gpsAvailable()) {
        gpsRequested = false;
        return;
    }
    const bool fix = gps->hasFix();
    if (fix && (!fixSeen || gps->p.timestamp != lastFixTime)) {
        fixSeen = true;
        lastFixTime = gps->p.timestamp;
        lastFixSeenMs = millis();
    }
    if (fix != lastFix) {
        lastFix = fix;
        emitGpsLine(false);
    } else if (gpsRequested && fix && gps->p.timestamp != gpsRequestFixTime) {
        emitGpsLine(false);
    } else if (gpsRequested && !Throttle::isWithinTimespanMs(gpsRequestMs, GPS_REQUEST_TIMEOUT_MS)) {
        emitGpsLine(true);
    } else {
        Throttle::execute(&lastGpsLineMs, fcuPresent ? GPS_LINE_FCU_INTERVAL_MS : GPS_LINE_INTERVAL_MS, emitGpsLineNow);
    }
#endif
}

void setGpsIntervalCap(uint32_t capMs)
{
#if !MESHTASTIC_EXCLUDE_GPS
    GPSUpdateScheduling::intervalCapMs = capMs;
#else
    (void)capMs;
#endif
}

void updateFcuPresence()
{
    if (fcuPresent && !Throttle::isWithinTimespanMs(lastFcuMs, FCU_PRESENT_MS)) {
        fcuPresent = false;
        setGpsIntervalCap(0);
        LOG_INFO("FCU absent: no keep-alive for %u s, stock GPS interval restored", (unsigned)(FCU_PRESENT_MS / 1000));
    }
}

#if HAS_SCREEN
const char SYNC_BANNER[] = "FCU sync...";
const char SYNC_DONE_BANNER[] = "FCU sync complete";
// Safety expiry only; the session end normally replaces or clears the banner first.
constexpr uint32_t SYNC_BANNER_MAX_MS = 10 * 60 * 1000;
constexpr uint32_t SYNC_DONE_BANNER_MS = 3000;

bool syncBannerShowing()
{
    return screen && screen->isOverlayBannerShowing() &&
           strcmp(graphics::NotificationRenderer::alertBannerMessage, SYNC_BANNER) == 0;
}

void showSyncBanner()
{
#if !MESHTASTIC_EXCLUDE_NCTNL
    if (nctnlModule && nctnlModule->isAlertBannerActive())
        return;
#endif
    if (screen && !syncBannerShowing())
        screen->showSimpleBanner(SYNC_BANNER, SYNC_BANNER_MAX_MS);
}

// Leaves any other banner (for example an NCTNL alert that replaced ours) alone.
void endSyncBanner(bool complete)
{
    if (!syncBannerShowing())
        return;
    if (complete)
        screen->showSimpleBanner(SYNC_DONE_BANNER, SYNC_DONE_BANNER_MS);
    else
        graphics::NotificationRenderer::alertBannerUntil = 1; // expires on the next frame, like any timed banner
}
#else
void showSyncBanner() {}
void endSyncBanner(bool) {}
#endif

void updateSession()
{
    if (bannerShowPending) {
        bannerShowPending = false;
        showSyncBanner();
    }
    if (syncWaiting && !sessionActive && !Throttle::isWithinTimespanMs(syncWaitStartMs, SYNC_WAIT_MS)) {
        syncWaiting = false;
        endSyncBanner(false);
        LOG_INFO("FCU sync: no API session within %u s", (unsigned)(SYNC_WAIT_MS / 1000));
    }
    if (sessionEndPending) {
        sessionEndPending = false;
        endSyncBanner(sessionEndedByDisconnect);
        if (!sessionActive)
            helloPending = true;
    }
}

// Builds the current replay item into jsonBuf; false when the current phase has no more items.
bool buildReplayItem(char &type, size_t &len)
{
#if HAS_SCREEN
    if (replay == Replay::Chat) {
        const auto &messages = messageStore.getMessages();
        if (replayIndex >= messages.size())
            return false;
        const StoredMessage &sm = messages[replayIndex];
        NctnlFcuLine::ChatReplay m = {};
        m.from = sm.sender;
        m.to = sm.dest;
        m.ch = sm.channelIndex;
        m.t = sm.timestamp;
        m.bootRelative = sm.isBootRelative;
        m.txt = MessageStore::getText(sm);
        type = 'M';
        len = NctnlFcuLine::formatChatReplayJson(m, jsonBuf, sizeof(jsonBuf));
        return true;
    }
#endif
#if NCTNL_TEXT_STORE
    if (replay == Replay::Data) {
        const NctnlTextStore::Entry *e = NctnlTextStore::get(replayIndex);
        if (!e)
            return false;
        const NctnlFcuLine::DataReplay d = {e->from, e->to, e->channel, e->id, e->time, e->text};
        type = 'D';
        len = NctnlFcuLine::formatDataReplayJson(d, jsonBuf, sizeof(jsonBuf));
        return true;
    }
#endif
    return false;
}

// Caller holds mirrorLock. Returns true while replay lines remain.
bool stepReplay()
{
    while (replay != Replay::Idle) {
        char type = 0;
        size_t len = 0;
        if (!buildReplayItem(type, len)) {
            replay = replay == Replay::Chat ? Replay::Data : Replay::Idle;
            replayIndex = 0;
            continue;
        }
        if (len == 0) {
            writeError("len", type, "replay JSON too long");
            replayIndex++;
            continue;
        }
        const bool paced = !replayWaiting || Throttle::isWithinTimespanMs(replayWaitStartMs, REPLAY_MAX_WAIT_MS);
        const Emit result = writeLine(type, nullptr, reinterpret_cast<const uint8_t *>(jsonBuf), len, paced);
        if (result == Emit::NoRoom) {
            if (!replayWaiting) {
                replayWaiting = true;
                replayWaitStartMs = millis();
            }
            return true;
        }
        replayWaiting = false;
        if (result == Emit::Suppressed) {
            replay = Replay::Idle;
            return false;
        }
        replayIndex++;
    }
    return false;
}

void resetSessionState()
{
    bannerShowPending = false;
    sessionEndPending = false;
    helloPending = false;
    syncWaiting = false;
    replay = Replay::Idle;
    gpsRequested = false;
    if (fcuPresent) {
        fcuPresent = false;
        setGpsIntervalCap(0);
    }
}

int32_t tick()
{
    uptimeSeconds();
    if (!isActive()) {
        resetSessionState();
        return POWER_POLL_MS;
    }
    updateFcuPresence();
    updateSession();
    if (!canEmit())
        return POWER_POLL_MS;

    if (helloPending) {
        helloPending = false;
        lastHelloMs = millis();
        emitHello();
        replay = Replay::Chat;
        replayIndex = 0;
        replayWaiting = false;
    }
    if (nodeDB && nodeDB->getNodeNum() != 0) {
        const uint32_t helloInterval = uptimeSeconds() < HELLO_FAST_UPTIME_S ? HELLO_FAST_INTERVAL_MS : HELLO_INTERVAL_MS;
        Throttle::execute(&lastHelloMs, helloInterval, emitHello);
    }
    const uint8_t flags = powerFlags();
    if (flags != lastPowerFlags) {
        lastPowerFlags = flags;
        lastBatteryMs = 0;
    }
    Throttle::execute(&lastBatteryMs, BATTERY_INTERVAL_MS, emitBattery);
    updateGps();

    concurrency::LockGuard guard(mirrorLock);
    return stepReplay() ? REPLAY_POLL_MS : POWER_POLL_MS;
}

class MirrorThread : public concurrency::OSThread
{
  public:
    MirrorThread() : OSThread("NctnlFcuMirror") {}

  protected:
    int32_t runOnce() override { return tick(); }
};

void handleSync()
{
    const uint32_t now = millis();
    const bool ok = !syncEverAccepted || !Throttle::isWithinTimespanMs(lastSyncAcceptMs, SYNC_GUARD_MS);
    uint32_t retrySeconds = 0;
    if (ok) {
        syncEverAccepted = true;
        lastSyncAcceptMs = now;
        syncWaiting = true;
        syncWaitStartMs = now;
        bannerShowPending = true;
    } else {
        retrySeconds = (SYNC_GUARD_MS - (now - lastSyncAcceptMs) + 999) / 1000;
    }
    {
        concurrency::LockGuard guard(mirrorLock);
        if (canEmit())
            writeJson('K', "sync", NctnlFcuLine::formatSyncResponseJson(ok, retrySeconds, jsonBuf, sizeof(jsonBuf)),
                      "K JSON too long");
    }
    if (ok)
        LOG_INFO("FCU sync accepted, waiting %u s for an API session", (unsigned)(SYNC_WAIT_MS / 1000));
    else
        LOG_INFO("FCU sync refused, retry in %u s", (unsigned)retrySeconds);
    wake();
}

void handleGpsRequest()
{
#if !MESHTASTIC_EXCLUDE_GPS
    if (!gpsAvailable())
        return;
    if (gpsCommandSeen && Throttle::isWithinTimespanMs(lastGpsCommandMs, GPS_REPEAT_IGNORE_MS))
        return;
    gpsCommandSeen = true;
    lastGpsCommandMs = millis();
    gpsRequested = true;
    gpsRequestMs = lastGpsCommandMs;
    gpsRequestFixTime = gps->hasFix() ? gps->p.timestamp : 0;
    gps->requestFix();
    wake();
#endif
}
} // namespace

bool isActive()
{
    return !moduleConfig.nctnl.fcu_integration_disabled;
}

void begin()
{
    if (mirrorLock)
        return;
    mirrorLock = new concurrency::Lock();
    lastUptimeMs = millis();
    uptimeMs = lastUptimeMs;
    tick();
    mirrorThread = new MirrorThread();
}

void onLoraRx(const meshtastic_MeshPacket *p)
{
    if (!p || !canEmit())
        return;
    concurrency::LockGuard guard(mirrorLock);
    writePacket('R', *p);
}

void onTxComplete(const meshtastic_MeshPacket *p, uint32_t airtimeMs)
{
    if (!p || !canEmit())
        return;
    char airtime[NctnlFcuLine::MAX_EXTRA_LEN + 1];
    snprintf(airtime, sizeof(airtime), "%lu", (unsigned long)airtimeMs);
    concurrency::LockGuard guard(mirrorLock);
    writePacket('T', *p, airtime);
}

void onToPhone(const meshtastic_MeshPacket *p)
{
    if (!p || !canEmit())
        return;
    concurrency::LockGuard guard(mirrorLock);
    writeDecodedPacket('P', *p);
}

void onFromPhone(const meshtastic_MeshPacket &p)
{
    if (!canEmit())
        return;
    concurrency::LockGuard guard(mirrorLock);
    packetCopy = p;
    packetCopy.from = nodeDB->getNodeNum();
    if (isAdmin(packetCopy))
        packetCopy.decoded.payload.size = 0;
    writePacket('U', packetCopy);
}

void onApiFrame()
{
    if (!mirrorLock || !isActive() || sessionActive)
        return;
    sessionActive = true;
    syncWaiting = false;
    helloPending = false;
    replay = Replay::Idle;
    bannerShowPending = true;
    wake();
}

void onApiSessionEnd(bool disconnected)
{
    if (!mirrorLock || !sessionActive)
        return;
    sessionActive = false;
    sessionEndPending = true;
    sessionEndedByDisconnect = disconnected;
    wake();
}

void onCommand(const char *cmd)
{
    if (!mirrorLock || !isActive() || !cmd)
        return;
    if (strcmp(cmd, "fcu") == 0) {
        lastFcuMs = millis();
        if (!fcuPresent) {
            fcuPresent = true;
            setGpsIntervalCap(FCU_GPS_INTERVAL_MS);
            LOG_INFO("FCU present: GPS update interval capped at %u s (RAM only)", (unsigned)(FCU_GPS_INTERVAL_MS / 1000));
            wake();
        }
    } else if (strcmp(cmd, "sync") == 0) {
        handleSync();
    } else if (strcmp(cmd, "gps") == 0) {
        handleGpsRequest();
    }
}
} // namespace NctnlFcuMirror
#endif
