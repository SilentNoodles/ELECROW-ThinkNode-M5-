#pragma once

#include <cstdint>

namespace NctnlDevelopmentConfig {
// Development-only values until NCTNL has a user-accessible settings source.
inline constexpr bool NCTNL_ENABLED = true;
inline constexpr bool QUICK_MESSAGE_MENU_ENABLED = true;
inline constexpr bool AUTOMATIC_STATUS_UPDATES_ENABLED = true;
inline constexpr bool BATTERY_ALERTS_ENABLED = true;
inline constexpr bool DATA_EVENTS_ENABLED = true;
inline constexpr uint8_t BATTERY_LOW_PERCENT = 20;
inline constexpr uint8_t BATTERY_CRITICAL_PERCENT = 10;
inline constexpr uint8_t BATTERY_EMERGENCY_PERCENT = 5;
inline constexpr uint8_t BATTERY_REARM_MARGIN_PERCENT = 5;
inline constexpr uint32_t CHARGING_ALL_CLEAR_SECONDS = 60;
inline constexpr uint32_t ALERT_BANNER_SECONDS = 40;
inline constexpr uint32_t ALERT_REPEAT_SECONDS = 10;
inline constexpr const char *DATA_CHANNEL_NAME = "NCTNL_DATA";
inline constexpr const char *COMMS_CHANNEL_NAME = "NCTNL_COMMS";
inline constexpr const char *CTRL_CHANNEL_NAME = "NCTNL_CTRL";
} // namespace NctnlDevelopmentConfig
