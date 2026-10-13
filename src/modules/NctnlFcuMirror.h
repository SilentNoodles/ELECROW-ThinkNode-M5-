#pragma once

#if NCTNL_FCU_MIRROR
#include "mesh/generated/meshtastic/mesh.pb.h"

// Writes machine-readable "@NCTNL1" lines to the serial console for an attached FCU. Observe-only.
namespace NctnlFcuMirror
{
void begin();
bool isActive();
void onLoraRx(const meshtastic_MeshPacket *p);
void onTxComplete(const meshtastic_MeshPacket *p, uint32_t airtimeMs);
void onToPhone(const meshtastic_MeshPacket *p);
void onFromPhone(const meshtastic_MeshPacket &p);
// Serial console hooks: every accepted API frame, the end of the API session, and "@NCTNL1 C <cmd>" lines.
void onApiFrame();
void onApiSessionEnd(bool disconnected);
void onCommand(const char *cmd);
} // namespace NctnlFcuMirror
#endif
