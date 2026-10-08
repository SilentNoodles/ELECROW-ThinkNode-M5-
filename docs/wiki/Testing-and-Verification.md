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

| Test | Item | Result |
| --- | --- | --- |
| S4-01 | Stage 4 | Verified |
| S4-02 | Stage 4 | Verified |
| S4-03 | NCTNL Quiet, Online at start-up (PR #30) | Verified |
| — | Battery alerts, Emergency → Offline and charging restore on a real discharge | Pending |
| — | Quick Chat picker hides `NCTNL_DATA` / `NCTNL_CTRL` | Pending |

The NCTNL Settings crash fix (PR #28), the boot-time GPIO fix and NCTNL_DATA events have also been verified on the device.

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
