#pragma once

#include "SinglePortModule.h"

class NctnlModule : public SinglePortModule
{
  public:
    NctnlModule();

  protected:
    ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
};

extern NctnlModule *nctnlModule;