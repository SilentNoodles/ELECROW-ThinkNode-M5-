#include "NctnlModule.h"
#include "NodeDB.h"
#if defined(ELECROW_ThinkNode_M5)
#include "graphics/Screen.h"
#include "mesh/Throttle.h"
#include "mesh/http/WebServer.h"
#include <WiFi.h>
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
    if (webConfigActive && !Throttle::isWithinTimespanMs(webConfigStartedAt, 10 * 60 * 1000UL))
        stopWebConfig();
#endif
    return 250;
}

#if defined(ELECROW_ThinkNode_M5)
bool NctnlModule::startWebConfig()
{
    if (webConfigActive)
        return true;
    char ssid[20];
    char password[16];
    const uint16_t suffix = nodeDB->getNodeNum() & 0xffff;
    snprintf(ssid, sizeof(ssid), "NCTNL-M5-%04X", suffix);
    snprintf(password, sizeof(password), "Nctnl%04X!", suffix);
    WiFi.mode(config.network.wifi_enabled ? WIFI_AP_STA : WIFI_AP);
    if (!WiFi.softAP(ssid, password) || !initNctnlWebServer()) {
        WiFi.softAPdisconnect(true);
        LOG_ERROR("NCTNL Web Config failed to start");
        return false;
    }
    webConfigActive = true;
    webConfigStartedAt = millis();
    OSThread::enabled = true;
    char details[160];
    snprintf(details, sizeof(details), "NCTNL WEB CONFIG\nWi-Fi: %s\nPassword: %s\nOpen: 192.168.4.1", ssid, password);
    static const char *options[] = {"Stop"};
    graphics::BannerOverlayOptions banner;
    banner.message = details;
    banner.optionsArrayPtr = options;
    banner.optionsCount = 1;
    banner.bannerCallback = [this](int) { stopWebConfig(); };
    screen->showOverlayBanner(banner);
    LOG_INFO("NCTNL Web Config started at http://192.168.4.1");
    return true;
}

void NctnlModule::stopWebConfig()
{
    if (!webConfigActive)
        return;
    stopNctnlWebServer();
    WiFi.softAPdisconnect(true);
    if (!config.network.wifi_enabled)
        WiFi.mode(WIFI_OFF);
    webConfigActive = false;
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
