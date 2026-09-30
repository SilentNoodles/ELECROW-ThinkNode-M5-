#include "NctnlModule.h"
#include "NodeDB.h"
#if defined(ELECROW_ThinkNode_M5)
#include "graphics/Screen.h"
#include "mesh/Throttle.h"
#include "mesh/http/WebServer.h"
#include "mesh/wifi/WiFiAPClient.h"
#endif

NctnlModule *nctnlModule;

NctnlModule::NctnlModule()
    : SinglePortModule("nctnl", meshtastic_PortNum_PRIVATE_APP), concurrency::OSThread("NctnlWeb")
{
    LOG_INFO("NCTNL module loaded");
    disable();
}

int32_t NctnlModule::runOnce()
{
#if defined(ELECROW_ThinkNode_M5)
    if (restartAt && !Throttle::isWithinTimespanMs(restartAt, 1500))
        ESP.restart();
    if (webConfigStarting) {
        const TemporaryWifiApState state = getTemporaryWifiApState();
        if (state == TemporaryWifiApState::READY) {
            LOG_INFO("NCTNL Web Config WiFi/AP ready; starting HTTP server");
            if (!initNctnlWebServer()) {
                LOG_ERROR("NCTNL Web Config startup failed: HTTP server unavailable");
                screen->showSimpleBanner("Web Config unavailable", 2500);
                stopWebConfig();
            } else {
                webConfigStarting = false;
                webConfigActive = true;
                webConfigStartedAt = millis();
                char details[160];
                snprintf(details, sizeof(details), "NCTNL WEB CONFIG\nWi-Fi: %s\nPassword: %s\nOpen: 192.168.4.1",
                         webConfigSsid, webConfigPassword);
                static const char *options[] = {"Stop"};
                graphics::BannerOverlayOptions banner;
                banner.message = details;
                banner.optionsArrayPtr = options;
                banner.optionsCount = 1;
                banner.bannerCallback = [this](int) { stopWebConfig(); };
                screen->showOverlayBanner(banner);
                LOG_INFO("NCTNL Web Config HTTP server ready");
                LOG_INFO("NCTNL Web Config fully active at http://192.168.4.1");
            }
        } else if (state == TemporaryWifiApState::FAILED) {
            LOG_ERROR("NCTNL Web Config startup failed: WiFi/AP unavailable");
            screen->showSimpleBanner("Web Config unavailable", 2500);
            stopWebConfig();
        } else if (!Throttle::isWithinTimespanMs(webConfigRequestedAt, 15000)) {
            LOG_ERROR("NCTNL Web Config startup failed: WiFi/AP readiness timeout");
            screen->showSimpleBanner("Web Config unavailable", 2500);
            stopWebConfig();
        }
    }
    if (webConfigActive && !Throttle::isWithinTimespanMs(webConfigStartedAt, 10 * 60 * 1000UL))
        stopWebConfig();
#endif
    return 250;
}

#if defined(ELECROW_ThinkNode_M5)
bool NctnlModule::startWebConfig()
{
    if (webConfigActive || webConfigStarting)
        return true;
    const uint16_t suffix = nodeDB->getNodeNum() & 0xffff;
    snprintf(webConfigSsid, sizeof(webConfigSsid), "NCTNL-M5-%04X", suffix);
    snprintf(webConfigPassword, sizeof(webConfigPassword), "Nctnl%04X!", suffix);
    LOG_INFO("NCTNL Web Config requested");
    if (!startTemporaryWifiAp(webConfigSsid, webConfigPassword)) {
        LOG_ERROR("NCTNL Web Config startup failed: WiFi/AP request rejected");
        stopTemporaryWifiAp();
        return false;
    }
    webConfigStarting = true;
    webConfigRequestedAt = millis();
    OSThread::setIntervalFromNow(0);
    OSThread::enabled = true;
    return true;
}

void NctnlModule::stopWebConfig()
{
    if (!webConfigActive && !webConfigStarting && getTemporaryWifiApState() == TemporaryWifiApState::INACTIVE)
        return;
    stopNctnlWebServer();
    stopTemporaryWifiAp();
    webConfigActive = false;
    webConfigStarting = false;
    disable();
    LOG_INFO("NCTNL Web Config stopped");
}

void NctnlModule::scheduleWebConfigRestart()
{
    restartAt = millis();
}
#endif

bool NctnlModule::isEnabled() const
{
    return moduleConfig.nctnl.enabled;
}

bool NctnlModule::isQuickMenuEnabled() const
{
    return isEnabled() && moduleConfig.nctnl.quick_menu_enabled;
}

bool NctnlModule::wantPacket(const meshtastic_MeshPacket *p)
{
    return isEnabled() && SinglePortModule::wantPacket(p);
}

ProcessMessage NctnlModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    if (!isEnabled()) {
        return ProcessMessage::CONTINUE;
    }
    return ProcessMessage::CONTINUE;
}
