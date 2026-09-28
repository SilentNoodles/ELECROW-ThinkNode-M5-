#include "NctnlModule.h"

NctnlModule *nctnlModule;

NctnlModule::NctnlModule()
    : SinglePortModule("nctnl", meshtastic_PortNum_PRIVATE_APP)
{
    LOG_INFO("NCTNL module loaded");
}

ProcessMessage NctnlModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    return ProcessMessage::CONTINUE;
}
