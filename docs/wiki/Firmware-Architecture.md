# Firmware Architecture

This page provides the high-level map of the current NCTNL firmware implementation.

## NctnlModule

Primary files:

- `src/modules/NctnlModule.cpp`
- `src/modules/NctnlModule.h`

Responsibilities currently include:

- NCTNL module registration;
- effective feature-state helpers;
- NCTNL status-update logic;
- read-only NCTNL development-settings status page;
- structured NCTNL event encoding/transmission; and
- structured NCTNL event receive/decode handling.

The module currently uses Meshtastic `PRIVATE_APP` for its structured packet path.

## NctnlDevelopmentConfig

File:

`src/modules/NctnlDevelopmentConfig.h`

This contains temporary central development feature flags used while a final user-accessible settings model is not available.

## CannedMessageModule integration

Files:

- `src/modules/CannedMessageModule.cpp`
- `src/modules/CannedMessageModule.h`

NCTNL reuses this existing Meshtastic UI/message infrastructure for:

- Quick Message Menu;
- destination selection;
- predefined text messages;
- Free Text;
- Send Coordinates; and
- post-send UI lifecycle.

## StatusMessageModule integration

Files:

- `src/modules/StatusMessageModule.cpp`
- `src/modules/StatusMessageModule.h`

NCTNL delegates status persistence and advertising to the existing Meshtastic Status Message module.

This avoids creating a parallel NCTNL status mechanism.

## Module registration

`src/modules/Modules.cpp` includes and constructs the NCTNL module when it is not compile-time excluded.

## Persistence and client synchronisation

Relevant integration includes:

- `src/mesh/NodeDB.cpp`
- `src/mesh/PhoneAPI.cpp`

These areas support NCTNL module configuration persistence and transfer through existing Meshtastic configuration mechanisms.

## Protobufs

Important files include:

- `protobufs/meshtastic/nctnl.proto`
- `protobufs/meshtastic/module_config.proto`
- associated generated nanopb sources under `src/mesh/generated/meshtastic/`

## ThinkNode M5 build configuration

The variant configuration is:

`variants/esp32s3/ELECROW-ThinkNode-M5/platformio.ini`

A significant NCTNL-related change removes the inherited `MESHTASTIC_EXCLUDE_STATUS=1` flag using PlatformIO `build_unflags`, allowing the standard Meshtastic Status Message module to exist at runtime on the ThinkNode M5.
