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
- Quick Message Menu; and
- automatic status updates.

## Settings/status page

The current NCTNL settings/status page is informational and read-only.

It displays effective development settings but does not currently provide a supported user-editable configuration interface.

## Historical note

A custom Android configuration implementation was considered but rejected because maintaining a custom Meshtastic Android application would create an unnecessary long-term maintenance burden.

An M5-hosted Web Config proof of concept was also developed and later abandoned after physical clients could not communicate successfully with the advertised local AP/HTTP service.
