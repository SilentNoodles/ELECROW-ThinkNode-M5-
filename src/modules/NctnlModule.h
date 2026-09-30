#pragma once

#include "SinglePortModule.h"
#include "mesh/generated/meshtastic/nctnl.pb.h"

class NctnlModule : public SinglePortModule
{
  public:
    NctnlModule();
    bool isEnabled() const;
    bool isQuickMenuEnabled() const;
    bool sendEvent(meshtastic_NctnlEvent_Type type, NodeNum dest, ChannelIndex channel);
#if defined(ELECROW_ThinkNode_M5) && HAS_SCREEN
    void showSettingsStatusPage() const;
#endif

  protected:
    bool wantPacket(const meshtastic_MeshPacket *p) override;
    ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
};

extern NctnlModule *nctnlModule;
