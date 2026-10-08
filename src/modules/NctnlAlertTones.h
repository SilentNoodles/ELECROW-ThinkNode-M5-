#pragma once

namespace NctnlAlertTones
{
// Only BATTERY_LOW and BATTERY_CRITICAL are used so far; the rest are reference only.
inline constexpr const char *SOS_RECEIVED =
    "PoliceSi:d=4,o=6,b=140:8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c,8e,8c";
inline constexpr const char *SOS_SENT =
    "Sos:d=16,o=6,b=160:g,p,g,p,g,4p,8g.,p,8g.,p,8g.,4p,g,p,g,p,g,1p,g,p,g,p,g,4p,8g.,p,8g.,p,8g.,4p,g,p,g,p,g";
inline constexpr const char *ASSISTANCE_RECEIVED = "siren:d=8,o=5,b=100:d,e,d,e,d,e,d,e";
inline constexpr const char *BATTERY_LOW = "Warning:d=4,o=5,b=180:a,8p,a,8p,a,8p,a";
inline constexpr const char *BATTERY_CRITICAL = "BatteryLow:d=4,o=5,b=110:f,8p,f,8p,f,8p,f";
inline constexpr const char *LOCATOR_BEACON =
    "RingRing:d=4,o=5,b=100:32b,32d6,32g6,32g6,32g6,8p,32b,32d6,32g6,32g6,32g6,2p,32b,32d6,32g6,32g6,32g6,8p,32b,32d6,32g6,32g6,"
    "32g6,2p,32b,32d6,32g6,32g6,32g6,8p,32b,32d6,32g6,32g6,32g6";
inline constexpr const char *ACTION_CONFIRM = "Pling:d=16,o=6,b=140:e6,32p,d6";
inline constexpr const char *ACTION_FAILED = "Pling3:d=16,o=5,b=120:g6,32p,g4";
} // namespace NctnlAlertTones
