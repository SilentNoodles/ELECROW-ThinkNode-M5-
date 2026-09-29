#pragma once

#include "SinglePortModule.h"
#include "concurrency/OSThread.h"

class NctnlModule : public SinglePortModule, private concurrency::OSThread
{
  public:
    NctnlModule();
    bool isEnabled() const;
    bool isQuickMenuEnabled() const;
#if defined(ELECROW_ThinkNode_M5)
    bool startWebConfig();
    void stopWebConfig();
    void scheduleWebConfigRestart();
#endif

  protected:
    bool wantPacket(const meshtastic_MeshPacket *p) override;
    ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
    int32_t runOnce() override;
#if defined(ELECROW_ThinkNode_M5)
    bool webConfigActive = false;
    uint32_t webConfigStartedAt = 0;
    uint32_t restartAt = 0;
#endif
};

extern NctnlModule *nctnlModule;
