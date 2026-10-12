#include "configuration.h"

#if NCTNL_FCU_MIRROR
#include "Channels.h"
#include "NctnlFcuLine.h"
#include "NctnlFcuMirror.h"
#include "NodeDB.h"
#include "PowerStatus.h"
#include "SerialConsole.h"
#include "Throttle.h"
#include "concurrency/LockGuard.h"
#include "concurrency/OSThread.h"
#include "mesh-pb-constants.h"
#include <cstring>

namespace NctnlFcuMirror
{
namespace
{
constexpr uint32_t HELLO_INTERVAL_MS = 60 * 1000;
constexpr uint32_t BATTERY_INTERVAL_MS = 60 * 1000;
constexpr uint32_t POWER_POLL_MS = 1000;
constexpr size_t JSON_MAX = 384;
constexpr size_t PAYLOAD_MAX = meshtastic_MeshPacket_size > JSON_MAX ? meshtastic_MeshPacket_size : JSON_MAX;

concurrency::Lock *mirrorLock;
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

bool canEmit()
{
    return mirrorLock && isActive() && console && !console->isMirrorSuppressed();
}

// Caller holds mirrorLock.
void writeLine(char type, const char *extra, const uint8_t *payload, size_t len);

void writeError(const char *code, char forType, const char *reason)
{
    char text[48];
    const int n = snprintf(text, sizeof(text), "%c %s", forType, reason);
    if (n > 0 && (size_t)n < sizeof(text))
        writeLine('E', code, reinterpret_cast<const uint8_t *>(text), (size_t)n);
}

void writeLine(char type, const char *extra, const uint8_t *payload, size_t len)
{
    const size_t n = NctnlFcuLine::formatLine(type, seq, extra, payload, len, lineBuf, sizeof(lineBuf));
    if (n == 0) {
        if (type != 'E')
            writeError("len", type, "line too long");
        return;
    }
    switch (console->writeMirrorLine(reinterpret_cast<const uint8_t *>(lineBuf), n)) {
    case SerialConsole::MirrorWrite::Suppressed:
        return;
    case SerialConsole::MirrorWrite::Dropped:
        dropped++;
        break;
    case SerialConsole::MirrorWrite::Written:
        break;
    }
    seq++;
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
    const size_t len = NctnlFcuLine::formatHelloJson(h, jsonBuf, sizeof(jsonBuf));
    if (len == 0)
        writeError("len", 'H', "hello JSON too long");
    else
        writeLine('H', nullptr, reinterpret_cast<const uint8_t *>(jsonBuf), len);
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
    const size_t len = NctnlFcuLine::formatBatteryJson(mv, pct, powerStatus->getHasUSB(), powerStatus->getIsCharging(), jsonBuf,
                                                       sizeof(jsonBuf));
    if (len == 0)
        writeError("len", 'B', "battery JSON too long");
    else
        writeLine('B', nullptr, reinterpret_cast<const uint8_t *>(jsonBuf), len);
}

void tick()
{
    uptimeSeconds();
    if (!canEmit())
        return;
    if (nodeDB && nodeDB->getNodeNum() != 0)
        Throttle::execute(&lastHelloMs, HELLO_INTERVAL_MS, emitHello);
    const uint8_t flags = powerFlags();
    if (flags != lastPowerFlags) {
        lastPowerFlags = flags;
        lastBatteryMs = 0;
    }
    Throttle::execute(&lastBatteryMs, BATTERY_INTERVAL_MS, emitBattery);
}

class MirrorThread : public concurrency::OSThread
{
  public:
    MirrorThread() : OSThread("NctnlFcuMirror") {}

  protected:
    int32_t runOnce() override
    {
        tick();
        return POWER_POLL_MS;
    }
};
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
    new MirrorThread();
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
} // namespace NctnlFcuMirror
#endif
