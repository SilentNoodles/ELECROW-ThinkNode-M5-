#include "NctnlModule.h"
#include "Channels.h"
#include "MeshService.h"
#include "NctnlAlertTones.h"
#include "NctnlDevelopmentConfig.h"
#include "NodeDB.h"
#include "PowerStatus.h"
#include "RTC.h"
#include "Throttle.h"
#include "configuration.h"
#include "modules/ExternalNotificationModule.h"
#if !MESHTASTIC_EXCLUDE_STATUS
#include "StatusMessageModule.h"
#endif
#include <string>
#include <time.h>
#if HAS_SCREEN
#include "graphics/Screen.h"
#endif

namespace
{
constexpr uint32_t NCTNL_PROTOCOL_VERSION = 1;
constexpr int32_t ALERT_TONE_STEP_MS = 25;
constexpr int32_t MONITOR_POLL_MS = 1000;
constexpr uint32_t BATTERY_CHECK_MS = 10 * 1000;

constexpr int BATTERY_LEVEL_COUNT = 3;
constexpr uint8_t BATTERY_THRESHOLDS[BATTERY_LEVEL_COUNT] = {NctnlDevelopmentConfig::BATTERY_LOW_PERCENT,
                                                             NctnlDevelopmentConfig::BATTERY_CRITICAL_PERCENT,
                                                             NctnlDevelopmentConfig::BATTERY_EMERGENCY_PERCENT};
constexpr const char *BATTERY_EVENT_TYPES[BATTERY_LEVEL_COUNT] = {"low", "critical", "emergency"};
constexpr const char *BATTERY_BANNER_TITLES[BATTERY_LEVEL_COUNT] = {"Battery Low", "Battery Critical", "Battery Emergency"};
constexpr const char *BATTERY_TONES[BATTERY_LEVEL_COUNT] = {NctnlAlertTones::BATTERY_LOW, NctnlAlertTones::BATTERY_CRITICAL,
                                                            NctnlAlertTones::BATTERY_CRITICAL};

bool hasExternalPower()
{
    return powerStatus && (powerStatus->getHasUSB() || powerStatus->getIsCharging());
}

// Serial log lines are truncated at 160 characters, so long JSON is logged in parts.
constexpr int DATA_LOG_CHUNK = 100;

void formatIsoTimeOrNull(uint32_t epoch, char *out, size_t size)
{
    if (epoch == 0) {
        snprintf(out, size, "null");
        return;
    }
    const time_t t = epoch;
    struct tm tm;
    gmtime_r(&t, &tm);
    snprintf(out, size, "\"%04d-%02d-%02dT%02d:%02d:%02dZ\"", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
             tm.tm_sec);
}

void formatCoordinate(int32_t value, char *out, size_t size)
{
    const uint32_t magnitude = value < 0 ? 0u - static_cast<uint32_t>(value) : static_cast<uint32_t>(value);
    snprintf(out, size, "%s%lu.%07lu", value < 0 ? "-" : "", static_cast<unsigned long>(magnitude / 10000000UL),
             static_cast<unsigned long>(magnitude % 10000000UL));
}

void escapeJson(const char *in, char *out, size_t size)
{
    size_t o = 0;
    for (; *in != '\0' && o + 2 < size; in++) {
        if (*in == '"' || *in == '\\') {
            out[o++] = '\\';
        }
        out[o++] = *in;
    }
    out[o] = '\0';
}

std::string trim(const std::string &value)
{
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

const char *eventMessage(meshtastic_NctnlEvent_Type type)
{
    switch (type) {
    case meshtastic_NctnlEvent_Type_GOING_OFFLINE:
        return "NCTNL STATUS: Going Offline\nThis node is going offline and will no longer be available for communications until "
               "it returns online.";
    case meshtastic_NctnlEvent_Type_STANDBY:
        return "NCTNL STATUS: Going Standby\nThis node is entering standby. Communications remain available, but responses may be "
               "delayed.";
    case meshtastic_NctnlEvent_Type_CHECK_IN:
        return "NCTNL CHECK-IN: Status Confirmed\nThis node has checked in successfully. Everything is OK and no assistance is "
               "currently required.";
    case meshtastic_NctnlEvent_Type_ASSISTANCE:
        return "NCTNL ASSISTANCE: Assistance Requested\nNon-emergency assistance has been requested. Please respond when available "
               "to establish contact and determine what assistance is required.";
    case meshtastic_NctnlEvent_Type_ALL_CLEAR:
        return "NCTNL STATUS: All Clear\nThe previous situation has been resolved. No further assistance is currently required.";
    default:
        return nullptr;
    }
}
} // namespace

NctnlModule *nctnlModule;

NctnlModule::NctnlModule() : SinglePortModule("nctnl", meshtastic_PortNum_PRIVATE_APP), concurrency::OSThread("Nctnl")
{
    LOG_INFO("NCTNL module loaded");
}

bool NctnlModule::isEnabled() const
{
    return NctnlDevelopmentConfig::NCTNL_ENABLED;
}

bool NctnlModule::isQuickMenuEnabled() const
{
    return isEnabled() && NctnlDevelopmentConfig::QUICK_MESSAGE_MENU_ENABLED;
}

bool NctnlModule::areAutomaticStatusUpdatesEnabled() const
{
    return isEnabled() && NctnlDevelopmentConfig::AUTOMATIC_STATUS_UPDATES_ENABLED;
}

// Channels::getByName() falls back to the primary channel, which must never receive NCTNL traffic.
int NctnlModule::findChannelByName(const char *name) const
{
    if (name == nullptr || name[0] == '\0') {
        return -1;
    }
    for (ChannelIndex i = 0; i < channels.getNumChannels(); i++) {
        const meshtastic_Channel &ch = channels.getByIndex(i);
        if (ch.role != meshtastic_Channel_Role_DISABLED && strcasecmp(ch.settings.name, name) == 0) {
            return i;
        }
    }
    return -1;
}

bool NctnlModule::playAlertTone(const char *tone)
{
    if (tone == nullptr) {
        return false;
    }
    if (config.device.buzzer_mode == meshtastic_Config_DeviceConfig_BuzzerMode_DISABLED ||
        config.device.buzzer_mode == meshtastic_Config_DeviceConfig_BuzzerMode_SYSTEM_ONLY) {
        LOG_INFO("NCTNL alert tone skipped: buzzer mode %d", config.device.buzzer_mode);
        return false;
    }
    if (externalNotificationModule && externalNotificationModule->getMute()) {
        LOG_INFO("NCTNL alert tone skipped: muted");
        return false;
    }
    if (rtttl::isPlaying()) {
        LOG_INFO("NCTNL alert tone skipped: a ringtone is already playing");
        return false;
    }

    uint8_t pin = config.device.buzzer_gpio;
#ifdef PIN_BUZZER
    if (pin == 0) {
        pin = PIN_BUZZER;
    }
#endif
    if (pin == 0) {
        return false;
    }

    rtttl::begin(pin, tone);
    alertTonePlaying = true;
    setIntervalFromNow(0);
    return true;
}

int32_t NctnlModule::runOnce()
{
    // ExternalNotificationModule only advances rtttl while it is nagging, so drive our own tones here.
    if (alertTonePlaying) {
        if (rtttl::isPlaying()) {
            rtttl::play();
        } else {
            alertTonePlaying = false;
        }
    }

    if (isEnabled() && NctnlDevelopmentConfig::BATTERY_ALERTS_ENABLED) {
        if (!Throttle::isWithinTimespanMs(lastBatteryCheckMs, BATTERY_CHECK_MS)) {
            lastBatteryCheckMs = millis();
            checkBattery();
        }
        updateAlertToneRepeat();
    }
    return alertTonePlaying ? ALERT_TONE_STEP_MS : MONITOR_POLL_MS;
}

void NctnlModule::checkBattery()
{
    if (!powerStatus || !powerStatus->getHasBattery()) {
        return;
    }

    const uint8_t percent = powerStatus->getBatteryChargePercent();
    for (int i = 0; i < BATTERY_LEVEL_COUNT; i++) {
        if (percent > BATTERY_THRESHOLDS[i] + NctnlDevelopmentConfig::BATTERY_REARM_MARGIN_PERCENT) {
            batteryLevelArmed[i] = true;
        }
    }

    if (hasExternalPower()) {
        if (!onExternalPower) {
            onExternalPower = true;
            externalPowerSinceMs = millis();
        } else if (batteryAlertSinceAllClear &&
                   !Throttle::isWithinTimespanMs(externalPowerSinceMs,
                                                 NctnlDevelopmentConfig::CHARGING_ALL_CLEAR_SECONDS * 1000)) {
            LOG_INFO("NCTNL battery all-clear: external power for %u s",
                     static_cast<unsigned>(NctnlDevelopmentConfig::CHARGING_ALL_CLEAR_SECONDS));
            sendBatteryDataEvent("charging");
            for (int i = 0; i < BATTERY_LEVEL_COUNT; i++) {
                batteryLevelArmed[i] = true;
            }
            batteryAlertSinceAllClear = false;
        }
        return;
    }
    onExternalPower = false;

    // Most severe level first so only the highest newly crossed level fires.
    for (int level = BATTERY_LEVEL_COUNT - 1; level >= 0; level--) {
        if (batteryLevelArmed[level] && percent <= BATTERY_THRESHOLDS[level]) {
            for (int i = 0; i <= level; i++) {
                batteryLevelArmed[i] = false;
            }
            fireBatteryAlert(level, percent);
            break;
        }
    }
}

void NctnlModule::fireBatteryAlert(int level, uint8_t percent)
{
    LOG_WARN("NCTNL battery %s at %u%%", BATTERY_EVENT_TYPES[level], percent);
    batteryAlertSinceAllClear = true;
    sendBatteryDataEvent(BATTERY_EVENT_TYPES[level]);

#if HAS_SCREEN
    if (screen) {
        char banner[32];
        snprintf(banner, sizeof(banner), "%s\n%u%%", BATTERY_BANNER_TITLES[level], percent);
        screen->showSimpleBanner(banner, NctnlDevelopmentConfig::ALERT_BANNER_SECONDS * 1000);
    }
#endif

    repeatingAlertTone = BATTERY_TONES[level];
    lastAlertToneMs = millis();
    playAlertTone(repeatingAlertTone);
}

void NctnlModule::updateAlertToneRepeat()
{
    if (repeatingAlertTone == nullptr) {
        return;
    }

    bool bannerShowing = false;
#if HAS_SCREEN
    bannerShowing = screen && screen->isOverlayBannerShowing();
#endif
    if (!bannerShowing || hasExternalPower()) {
        repeatingAlertTone = nullptr;
        return;
    }
    if (!Throttle::isWithinTimespanMs(lastAlertToneMs, NctnlDevelopmentConfig::ALERT_REPEAT_SECONDS * 1000)) {
        lastAlertToneMs = millis();
        playAlertTone(repeatingAlertTone);
    }
}

bool NctnlModule::updateStatus(const char *status) const
{
    if (!areAutomaticStatusUpdatesEnabled()) {
        return false;
    }

#if MESHTASTIC_EXCLUDE_STATUS
    LOG_WARN("Skipping NCTNL status update because Status Message support is unavailable");
    return false;
#else
    const std::string existing = moduleConfig.statusmessage.node_status;
    std::string components[4];
    size_t start = 0;
    for (size_t i = 0; i < 4; ++i) {
        const size_t separator = existing.find('|', start);
        if ((i < 3 && separator == std::string::npos) || (i == 3 && separator != std::string::npos)) {
            LOG_WARN("Skipping NCTNL status update because the existing Status Message is not in NCTNL format");
            return false;
        }
        components[i] = trim(existing.substr(start, separator - start));
        start = separator + 1;
    }

    if (components[0].empty() || components[1].empty() || components[2].empty() || components[3] != "NCTNL.io" ||
        status == nullptr || trim(status).empty()) {
        LOG_WARN("Skipping NCTNL status update because the existing Status Message is not in NCTNL format");
        return false;
    }

    const std::string updated = components[0] + " | " + trim(status) + " | " + components[2] + " | NCTNL.io";
    if (statusMessageModule == nullptr) {
        LOG_WARN("Skipping NCTNL status update because the Status Message runtime instance is unavailable");
        return false;
    }
    if (!statusMessageModule->setStatusMessage(updated.c_str())) {
        LOG_WARN("Unable to apply NCTNL status update");
        return false;
    }
    LOG_INFO("NCTNL status update persisted and advertised");
    return true;
#endif
}

#if defined(ELECROW_ThinkNode_M5) && HAS_SCREEN
void NctnlModule::showSettingsStatusPage() const
{
    const char *channelNames[] = {NctnlDevelopmentConfig::DATA_CHANNEL_NAME, NctnlDevelopmentConfig::COMMS_CHANNEL_NAME,
                                  NctnlDevelopmentConfig::CTRL_CHANNEL_NAME};
    char channelTexts[3][4];
    for (size_t i = 0; i < 3; i++) {
        const int index = findChannelByName(channelNames[i]);
        if (index < 0) {
            snprintf(channelTexts[i], sizeof(channelTexts[i]), "-");
        } else {
            snprintf(channelTexts[i], sizeof(channelTexts[i]), "%d", index);
        }
    }

    static char status[280];
    snprintf(status, sizeof(status), "NCTNL Settings\nNCTNL:%s QMsg:%s\nStatus:%s Batt:%s\nDATA:%s COMMS:%s\nCTRL:%s Read-only",
             isEnabled() ? "On" : "Off", isQuickMenuEnabled() ? "On" : "Off", areAutomaticStatusUpdatesEnabled() ? "On" : "Off",
             isEnabled() && NctnlDevelopmentConfig::BATTERY_ALERTS_ENABLED ? "On" : "Off", channelTexts[0], channelTexts[1],
             channelTexts[2]);

    static const char *options[] = {"Back"};
    graphics::BannerOverlayOptions banner;
    banner.message = status;
    banner.optionsArrayPtr = options;
    banner.optionsCount = 1;
    banner.bannerCallback = [](int) {};
    screen->showOverlayBanner(banner);
}
#endif

bool NctnlModule::wantPacket(const meshtastic_MeshPacket *p)
{
    return isEnabled() && SinglePortModule::wantPacket(p);
}

bool NctnlModule::sendEvent(meshtastic_NctnlEvent_Type type, NodeNum dest, ChannelIndex channel)
{
    if (!isEnabled() || eventMessage(type) == nullptr) {
        return false;
    }

    meshtastic_MeshPacket *packet = allocDataPacket();
    meshtastic_NctnlEvent event = meshtastic_NctnlEvent_init_zero;
    event.protocol_version = NCTNL_PROTOCOL_VERSION;
    event.type = type;
    event.event_id = packet->id;

    packet->decoded.payload.size = pb_encode_to_bytes(packet->decoded.payload.bytes, sizeof(packet->decoded.payload.bytes),
                                                      meshtastic_NctnlEvent_fields, &event);
    if (packet->decoded.payload.size == 0) {
        packetPool.release(packet);
        return false;
    }

    packet->to = dest;
    packet->decoded.dest = dest;
    packet->channel = channel;
    packet->want_ack = true;
    service->sendToMesh(packet, RX_SRC_LOCAL, true);
    return true;
}

bool NctnlModule::sendStatusDataEvent(const char *type)
{
    return sendDataEvent(false, type);
}

bool NctnlModule::sendBatteryDataEvent(const char *type)
{
    return sendDataEvent(true, type);
}

bool NctnlModule::sendDataEvent(bool batteryEvent, const char *type)
{
    if (!isEnabled() || !NctnlDevelopmentConfig::DATA_EVENTS_ENABLED || type == nullptr) {
        return false;
    }

    const int channel = findChannelByName(NctnlDevelopmentConfig::DATA_CHANNEL_NAME);
    if (channel < 0) {
        LOG_WARN("NCTNL %s event not sent: channel %s not found", type, NctnlDevelopmentConfig::DATA_CHANNEL_NAME);
        return false;
    }

    meshtastic_MeshPacket *packet = allocDataPacket();
    const NodeNum node = nodeDB->getNodeNum();

    char name[sizeof(owner.short_name) * 2];
    escapeJson(owner.short_name, name, sizeof(name));
    char now[24];
    formatIsoTimeOrNull(getValidTime(RTCQualityDevice), now, sizeof(now));
    const unsigned battery = powerStatus ? powerStatus->getBatteryChargePercent() : 0;

    char json[meshtastic_Constants_DATA_PAYLOAD_LEN * 2];
    int len =
        snprintf(json, sizeof(json),
                 "{\"k\":\"%s\",\"v\":1,\"type\":\"%s\",\"id\":\"%08x-%08x\",\"node\":\"!%08x\",\"name\":\"%s\",\"time\":%s,"
                 "\"bat\":%u",
                 batteryEvent ? "bat" : "sts", type, node, packet->id, node, name, now, battery);

    if (len > 0 && len < static_cast<int>(sizeof(json)) && (localPosition.latitude_i != 0 || localPosition.longitude_i != 0)) {
        char lat[16], lon[16], fix[24];
        formatCoordinate(localPosition.latitude_i, lat, sizeof(lat));
        formatCoordinate(localPosition.longitude_i, lon, sizeof(lon));
        formatIsoTimeOrNull(localPosition.time, fix, sizeof(fix));
        len += snprintf(json + len, sizeof(json) - len, ",\"lat\":%s,\"lon\":%s,\"fix\":%s", lat, lon, fix);
    }
    if (len > 0 && len < static_cast<int>(sizeof(json)) && batteryEvent) {
        len += snprintf(json + len, sizeof(json) - len, ",\"mv\":%d,\"chg\":%s",
                        powerStatus ? powerStatus->getBatteryVoltageMv() : 0, hasExternalPower() ? "true" : "false");
    }
    if (len > 0 && len < static_cast<int>(sizeof(json))) {
        len += snprintf(json + len, sizeof(json) - len, "}");
    }

    if (len <= 0 || len >= static_cast<int>(sizeof(json)) || len > meshtastic_Constants_DATA_PAYLOAD_LEN) {
        LOG_ERROR("NCTNL %s event not sent: JSON exceeds %d bytes", type, meshtastic_Constants_DATA_PAYLOAD_LEN);
        packetPool.release(packet);
        return false;
    }

    memcpy(packet->decoded.payload.bytes, json, len);
    packet->decoded.payload.size = len;
    packet->to = NODENUM_BROADCAST;
    packet->channel = channel;
    packet->want_ack = true;

    LOG_INFO("NCTNL_DATA sent on channel %d (%d bytes)", channel, len);
    const int parts = (len + DATA_LOG_CHUNK - 1) / DATA_LOG_CHUNK;
    for (int i = 0; i < parts; i++) {
        const int partLen = (len - i * DATA_LOG_CHUNK) < DATA_LOG_CHUNK ? (len - i * DATA_LOG_CHUNK) : DATA_LOG_CHUNK;
        LOG_INFO("NCTNL_DATA %d/%d: %.*s", i + 1, parts, partLen, json + i * DATA_LOG_CHUNK);
    }
    service->sendToMesh(packet, RX_SRC_LOCAL, true);
    return true;
}

ProcessMessage NctnlModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    if (!isEnabled()) {
        return ProcessMessage::CONTINUE;
    }

    LOG_INFO("Received NCTNL packet from=0x%08x id=0x%08x channel=%u size=%u", mp.from, mp.id, mp.channel,
             mp.decoded.payload.size);
    return ProcessMessage::CONTINUE;
}
