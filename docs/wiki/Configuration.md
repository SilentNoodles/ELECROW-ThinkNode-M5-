# Configuration

NCTNL currently has both a persistent configuration architecture and a temporary development-time control model.

## Persistent architecture

The firmware includes NCTNL module-configuration protobuf plumbing integrated with Meshtastic configuration persistence and synchronisation.

This architecture is intended to remain available for future user-accessible configuration.

## Current development controls

During active development, effective feature enablement is currently centralised in:

`src/modules/NctnlDevelopmentConfig.h`

Current development flags enable:

- NCTNL;
- Quick Message Menu;
- automatic status updates;
- battery alerts and NCTNL_DATA events; and
- the `NCTNL_DATA`, `NCTNL_COMMS` and `NCTNL_CTRL` channel names.

## Build-time defaults (`userPrefs.jsonc`)

`USERPREFS_RINGTONE_NAG_SECS` is set to `1`, so the message notification nag lasts 1 s. It is a default only: it applies on a fresh install or factory reset. A device with a stored external notification config keeps its existing nag timeout.

## FCU serial mirror

`NctnlConfig.fcu_integration_disabled` (field 3) controls the FCU serial mirror on the ThinkNode M5 build (`NCTNL_FCU_MIRROR`). The naming is inverted so an unset value means **on**: the mirror is on by default, including on devices that already have a saved config. There is no on-device toggle yet; it will come with the NCTNL Settings menu. The line format is in `docs/nctnl-fcu-mirror-spec.md`.

## Settings/status page

The current NCTNL settings/status page is informational and read-only.

It displays effective development settings but does not currently provide a supported user-editable configuration interface.

```text
NCTNL Settings
NCTNL:On QMsg:On
Stat:On Batt:On
Quiet:Off FCU:On
DATA:0 COM:1 CTL:2
```

## Historical note

A custom Android configuration implementation was considered but rejected because maintaining a custom Meshtastic Android application would create an unnecessary long-term maintenance burden.

An M5-hosted Web Config proof of concept was also developed and later abandoned after physical clients could not communicate successfully with the advertised local AP/HTTP service.
