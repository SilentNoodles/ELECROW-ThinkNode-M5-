#include "NctnlModule.h"
#include "MeshService.h"
#include "NodeDB.h"
#include "configuration.h"
#if HAS_SCREEN
#include "graphics/Screen.h"
#endif

namespace
{
constexpr uint32_t NCTNL_PROTOCOL_VERSION = 1;

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
    return moduleConfig.nctnl.enabled;
}

bool NctnlModule::isQuickMenuEnabled() const
{
    return isEnabled() && moduleConfig.nctnl.quick_menu_enabled;
}

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
