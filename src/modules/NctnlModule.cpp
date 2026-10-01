#include "NctnlModule.h"
#include "MeshService.h"
#include "NctnlDevelopmentConfig.h"
#include "NodeDB.h"
#include "configuration.h"
#if !MESHTASTIC_EXCLUDE_STATUS
#include "StatusMessageModule.h"
#endif
#include <string>
#if HAS_SCREEN
#include "graphics/Screen.h"
#endif

namespace
{
constexpr uint32_t NCTNL_PROTOCOL_VERSION = 1;

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

NctnlModule::NctnlModule()
    : SinglePortModule("nctnl", meshtastic_PortNum_PRIVATE_APP)
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
    if (statusMessageModule == nullptr || !statusMessageModule->setStatusMessage(updated.c_str())) {
        LOG_WARN("Unable to apply NCTNL status update");
        return false;
    }
    return true;
#endif
}

#if defined(ELECROW_ThinkNode_M5) && HAS_SCREEN
void NctnlModule::showSettingsStatusPage() const
{
    static char status[280];
    snprintf(status, sizeof(status),
             "Development settings\n\nNCTNL Enabled: %s\nQuick Message Menu: %s\nAutomatic Status Updates: %s\n\nValues shown "
             "here are currently hard-coded\nand cannot be changed from this page.",
             isEnabled() ? "Enabled" : "Disabled", isQuickMenuEnabled() ? "Enabled" : "Disabled",
             areAutomaticStatusUpdatesEnabled() ? "Enabled" : "Disabled");

    static const char *options[] = {"Back"};
    graphics::BannerOverlayOptions banner;
    banner.message = status;
    banner.optionsArrayPtr = options;
    banner.optionsCount = 1;
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

ProcessMessage NctnlModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    if (!isEnabled()) {
        return ProcessMessage::CONTINUE;
    }

    meshtastic_NctnlEvent event = meshtastic_NctnlEvent_init_zero;
    if (!pb_decode_from_bytes(mp.decoded.payload.bytes, mp.decoded.payload.size, meshtastic_NctnlEvent_fields, &event) ||
        event.protocol_version != NCTNL_PROTOCOL_VERSION || event.event_id == 0) {
        LOG_WARN("Ignoring invalid NCTNL event");
        return ProcessMessage::CONTINUE;
    }

    const char *message = eventMessage(event.type);
    if (message == nullptr) {
        LOG_WARN("Ignoring unsupported NCTNL event type %d", event.type);
        return ProcessMessage::CONTINUE;
    }

    LOG_INFO("Received NCTNL event type=%d id=0x%08x from=0x%08x", event.type, event.event_id, mp.from);
#if HAS_SCREEN
    if (screen) {
        screen->showSimpleBanner(message, 10000);
    }
#endif
    return ProcessMessage::CONTINUE;
}
