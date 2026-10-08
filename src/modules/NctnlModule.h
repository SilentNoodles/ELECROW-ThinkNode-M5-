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
    bool isQuiet() const;
    void handleStatusAction(const char *status);
    bool sendEvent(meshtastic_NctnlEvent_Type type, NodeNum dest, ChannelIndex channel);
    int findChannelByName(const char *name) const;
    bool playAlertTone(const char *tone);
    bool sendStatusDataEvent(const char *type);
    bool sendBatteryDataEvent(const char *type);
    bool isDataChannelText(const meshtastic_MeshPacket &mp) const;
    bool handleDataText(const meshtastic_MeshPacket &mp);
#if defined(ELECROW_ThinkNode_M5) && HAS_SCREEN
    void showSettingsStatusPage() const;
#endif

  protected:
    bool wantPacket(const meshtastic_MeshPacket *p) override;
    ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
    int32_t runOnce() override;

  private:
    bool sendDataEvent(bool batteryEvent, const char *type);
    void checkBattery();
    void fireBatteryAlert(int level, uint8_t percent);
    void updateAlertToneRepeat();
    void startRepeatingAlert(const char *banner, const char *tone, bool stopOnExternalPower);
    void setStatusState(const char *status);
    void announceStartupOnline();

    bool alertTonePlaying = false;
    const char *repeatingAlertTone = nullptr;
    bool repeatStopsOnExternalPower = false;
    uint32_t lastAlertToneMs = 0;
    uint32_t lastBatteryCheckMs = 0;
    bool batteryLevelArmed[3] = {true, true, true};
    bool batteryAlertSinceAllClear = false;
    bool onExternalPower = false;
    uint32_t externalPowerSinceMs = 0;
    // RAM-only; never persisted.
    bool quiet = false;
    char currentStatus[24] = "";
    uint32_t startupMs = 0;
    bool startupOnlineDone = false;
};

extern NctnlModule *nctnlModule;
