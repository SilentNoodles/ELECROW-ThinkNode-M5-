#include "NctnlModule.h"
#include "NodeDB.h"

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

ProcessMessage NctnlModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    if (!isEnabled()) {
        return ProcessMessage::CONTINUE;
    }
    return ProcessMessage::CONTINUE;
}
