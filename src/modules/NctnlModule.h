#pragma once

#include "SinglePortModule.h"
#include "concurrency/OSThread.h"
#include "mesh/generated/meshtastic/nctnl.pb.h"

class NctnlModule : public SinglePortModule, private concurrency::OSThread
{
  public:
    NctnlModule();
    bool isEnabled() const;
    bool isQuickMenuEnabled() const;
    bool areAutomaticStatusUpdatesEnabled() const;
    bool updateStatus(const char *status) const;
    bool sendEvent(meshtastic_NctnlEvent_Type type, NodeNum dest, ChannelIndex channel);
    int findChannelByName(const char *name) const;
    bool playAlertTone(const char *tone);
    bool sendStatusDataEvent(const char *type);
    bool sendBatteryDataEvent(const char *type);
#if defined(ELECROW_ThinkNode_M5) && HAS_SCREEN
    void showSettingsStatusPage() const;
#endif

  protected:
    bool wantPacket(const meshtastic_MeshPacket *p) override;
    ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
    int32_t runOnce() override;

  private:
    bool sendDataEvent(bool batteryEvent, const char *type);

    bool alertTonePlaying = false;
};

extern NctnlModule *nctnlModule;
