# Testing and Verification

NCTNL documentation distinguishes implementation from physical verification.

## Status definitions

### Implemented and physically verified

The code exists and the behaviour has been confirmed on physical hardware.

### Implemented but not physically verified

The code exists, but the relevant behaviour has not yet been confirmed on the real target device.

### Experimental

Prototype behaviour which may change substantially or be removed.

### Planned

Design intent only. This must not be described as current functionality.

### Deprecated / abandoned

Previously attempted or implemented work which is no longer part of the current design.

## Current important physical verifications

Physical testing has confirmed, among other items:

- NCTNL module startup on the ThinkNode M5;
- primary and alternate physical button mappings;
- Quick Message Menu operation;
- normal Meshtastic text transmission for the five predefined actions;
- automatic Status Message changes;
- live status refresh after the final local-client notification fix; and
- correct Send Coordinates post-send UI cleanup.

## Stage 4

| Build | Item | Result |
| --- | --- | --- |
| S4-01 | First Stage 4 build. `NCTNL_DATA` JSON v1 events correct; implicit ACK by rebroadcast seen; missing-channel fail-safe works | Verified |
| S4-02 | Data events moved to `TEXT_MESSAGE_APP`; `NCTNL_DATA` text suppressed on Field Nodes (still relayed); Assistance Request received banner and repeating siren; Mute silences it; boot-time invalid GPIO message gone | Verified |
| S4-03 | NCTNL Quiet (Standby/Offline mute broadcast text; DMs, assistance and battery alerts still sound); Online at start-up and after reboot; manual Mute unchanged (PR #30) | Verified |
| — | Battery alerts, Emergency → Offline and charging restore on a real discharge | Pending |
| — | Quick Chat picker hides `NCTNL_DATA` / `NCTNL_CTRL` | Pending |

The NCTNL Settings crash fix (PR #28) has also been verified on the device.

## Regression priorities

Future regression testing should cover:

- normal Meshtastic messaging;
- NCTNL menu launch;
- channel and direct-node destinations;
- all five predefined messages;
- status mappings;
- malformed/non-NCTNL status protection;
- Free Text;
- valid coordinate send;
- no-fix coordinate behaviour;
- post-send UI state;
- structured-event compatibility;
- settings/status page;
- boot branding; and
- ThinkNode M5 button behaviour.
