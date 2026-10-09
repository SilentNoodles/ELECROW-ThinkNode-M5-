#pragma once

#if NCTNL_FCU_MIRROR
#include "mesh/generated/meshtastic/mesh.pb.h"

// Writes machine-readable "@NCTNL1" lines to the serial console for an attached FCU. Observe-only.
namespace NctnlFcuMirror
{
void begin();
bool isActive();
void onLoraRx(const meshtastic_MeshPacket *p);
void onTxComplete(const meshtastic_MeshPacket *p);
void onToPhone(const meshtastic_MeshPacket *p);
void onFromPhone(const meshtastic_MeshPacket &p);
} // namespace NctnlFcuMirror
#endif
