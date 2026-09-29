#include "NctnlModule.h"
#include "NodeDB.h"

NctnlModule *nctnlModule;

NctnlModule::NctnlModule()
    : SinglePortModule("nctnl", meshtastic_PortNum_PRIVATE_APP)
{
    LOG_INFO("NCTNL module loaded");
#ifdef ELECROW_ThinkNode_M5
    LOG_INFO("NCTNL Stage 3A TEST: forcing NCTNL and Quick Message Menu enabled");
#endif
}

bool NctnlModule::isEnabled() const
{
#ifdef ELECROW_ThinkNode_M5
    // Temporary Stage 3A hardware-test override; do not persist this value.
    return true;
#endif
    return moduleConfig.nctnl.enabled;
}

bool NctnlModule::isQuickMenuEnabled() const
{
#ifdef ELECROW_ThinkNode_M5
    // Temporary Stage 3A hardware-test override; do not persist this value.
    return true;
#endif
    return isEnabled() && moduleConfig.nctnl.quick_menu_enabled;
}

bool NctnlModule::wantPacket(const meshtastic_MeshPacket *p)
{
    return isEnabled() && SinglePortModule::wantPacket(p);
}

ProcessMessage NctnlModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    if (!isEnabled()) {
        return ProcessMessage::CONTINUE;
    }
    return ProcessMessage::CONTINUE;
}
