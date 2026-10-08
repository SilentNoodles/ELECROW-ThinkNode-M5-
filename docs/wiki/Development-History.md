# Development History

NCTNL development history is retained because the reasons behind a change are often as important as the final code.

## Initial NCTNL module

The project first introduced the NCTNL module, registered it at startup and physically confirmed the module startup log on the ThinkNode M5.

## Persistent configuration

NCTNL module configuration was then integrated into Meshtastic's existing protobuf, persistence and client-synchronisation mechanisms.

## Physical button mapping

ThinkNode M5 physical testing confirmed:

- GPIO21 = Primary / UserButton
- GPIO14 = Alternate / BackButton

Temporary diagnostics used during verification were removed after the mappings were established.

## Quick Message development

The NCTNL Quick Message Menu was introduced with destination selection and Free Text support.

An early implementation routed predefined NCTNL actions through structured `PRIVATE_APP` events.

Physical and architectural review showed that this was the wrong transport for human-facing status messages. The predefined actions were therefore changed to normal Meshtastic text messages.

## Web Config experiment

A local ThinkNode M5 Web Config approach was explored.

Development included AP lifecycle work, HTTP handlers and menu integration. Despite the device reporting AP/DHCP/HTTP startup, real clients could not communicate successfully with the service.

The approach was abandoned rather than carrying a fragile network-management subsystem forward.

## Development configuration

NCTNL feature enablement was centralised in `NctnlDevelopmentConfig.h` while the longer-term user configuration approach remains under development.

## Automatic status updates

The five predefined Quick Messages were mapped to NCTNL status values while preserving Rough Location and Role.

### Status module exclusion problem

Physical testing showed that the status logic did not work despite apparently correct source integration.

Investigation found that the ThinkNode M5 inherited:

`MESHTASTIC_EXCLUDE_STATUS=1`

This meant the standard `StatusMessageModule` did not exist at runtime.

An initial attempt to override the flag with `-U/-D` PlatformIO build flags caused a build problem.

The successful fix used PlatformIO `build_unflags` to remove the inherited exclusion cleanly.

Physical testing then confirmed automatic status changes.

### Live refresh problem

A second issue remained. The changed status persisted and propagated, but the local UI/client could remain stale until the status settings page was opened.

The Status Message send path was changed so locally generated updates are also delivered to the connected client.

Physical testing confirmed live refresh.

## Send Coordinates UI cleanup

Send Coordinates transmitted correctly but could leave the device stuck on the outgoing-message screen.

The same proven post-send cleanup used by predefined NCTNL Quick Messages was applied.

Physical testing confirmed the issue was resolved.

## Stage 4 fixes

- **NCTNL Settings reboot** (PR #28): opening NCTNL Settings caused an abort from an empty `std::function` in the banner callback path. The page is now queued via `menuQueue` with a no-op banner callback. Verified on the device.
- **Boot-time invalid GPIO message**: fixed by removing `PIN_POWER_EN -1` from the ThinkNode M5 variant. Verified on the device.
- **Battery telemetry 101%**: traced to upstream Meshtastic `DeviceTelemetry`, which reports 101 while on USB or charging. Not an NCTNL bug.

## Current verified milestone

Current physically verified build:

**2.7.26.9165331**

Integration branch:

`feature/nctnl-stage1-completion`
