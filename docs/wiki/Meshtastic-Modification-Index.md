# Meshtastic Modification Index

This page is intended to act as the maintenance checklist when NCTNL is rebased or upgraded to a newer Meshtastic firmware version.

## Added NCTNL-specific files

| File | Purpose |
| --- | --- |
| `src/modules/NctnlModule.cpp` | NCTNL module implementation |
| `src/modules/NctnlModule.h` | NCTNL module interface |
| `src/modules/NctnlDevelopmentConfig.h` | Temporary central development configuration |
| `protobufs/meshtastic/nctnl.proto` | Structured NCTNL event protobuf |
| `src/mesh/generated/meshtastic/nctnl.pb.cpp` | Generated nanopb implementation |
| `src/mesh/generated/meshtastic/nctnl.pb.h` | Generated nanopb definitions |
| `src/graphics/img/icon_meshtastic.xbm` | Preserved Meshtastic logo asset |

## Modified upstream areas

### `src/configuration.h`

Introduces compile-time handling for the NCTNL module.

### `src/modules/Modules.cpp`

Registers/constructs the NCTNL module under the appropriate build guard.

### `src/modules/CannedMessageModule.cpp/.h`

Integrates NCTNL Quick Message behaviour, destination reuse, Free Text, coordinate sending and UI cleanup.

### `src/modules/StatusMessageModule.cpp/.h`

Provides the reusable status setter path and local-client refresh behaviour used by NCTNL.

### `src/mesh/NodeDB.cpp`

Includes NCTNL module configuration in the existing persistence model.

### `src/mesh/PhoneAPI.cpp`

Includes NCTNL module configuration in normal client/module configuration synchronisation.

### `src/graphics/draw/MenuHandler.cpp`

Contains NCTNL-specific menu integration.

### `src/graphics/draw/UIRenderer.cpp`

Contains NCTNL branding/UI changes.

### `src/graphics/img/icon.xbm`

Contains the active NCTNL boot image.

### `variants/esp32s3/ELECROW-ThinkNode-M5/platformio.ini`

Contains ThinkNode M5 NCTNL-related build configuration, including removal of the inherited Status Message exclusion.

### Protobuf configuration and generated files

NCTNL configuration fields are integrated with the existing Meshtastic module configuration and generated nanopb code.

## Upgrade rule

When moving NCTNL to a newer Meshtastic firmware base, each entry above should be reviewed against upstream changes rather than blindly copied.

The objective is to preserve NCTNL behaviour while retaining as much upstream Meshtastic code as possible.
