#include "NctnlModule.h"

NctnlModule *nctnlModule;

NctnlModule::NctnlModule()
    : SinglePortModule("nctnl", meshtastic_PortNum_PRIVATE_APP)
{
}

ProcessMessage NctnlModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    return ProcessMessage::CONTINUE;
}