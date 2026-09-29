#pragma once

#include "SinglePortModule.h"

class NctnlModule : public SinglePortModule
{
  public:
    NctnlModule();
    bool isEnabled() const;
    bool isQuickMenuEnabled() const;

  protected:
    bool wantPacket(const meshtastic_MeshPacket *p) override;
    ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
};

extern NctnlModule *nctnlModule;
