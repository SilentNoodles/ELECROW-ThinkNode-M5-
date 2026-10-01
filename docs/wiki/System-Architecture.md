# System Architecture

NCTNL is an extension layer over Meshtastic.

## Meshtastic remains the network foundation

NCTNL deliberately retains standard Meshtastic behaviour for:

- LoRa radio communication;
- mesh routing;
- encryption;
- channels;
- node database;
- normal messaging; and
- compatible client communication.

## NCTNL layer

NCTNL adds operational functions on top of those capabilities.

Where possible, NCTNL reuses established Meshtastic components rather than creating parallel systems.

Current examples include:

- Quick Messages using the normal text-message path;
- destination selection reusing existing Meshtastic channel/node handling;
- Node Status reusing Status Message;
- location sharing using normal text transport; and
- configuration persistence using existing module-configuration infrastructure.

## Node roles

### Field Node

The current actively developed role. The ThinkNode M5 is the primary field-node hardware.

### Relay Node

Planned role. Detailed behaviour is not yet final and should not be inferred.

### Base Station

A separate active workstream. Its architecture is not yet final.

## Architectural rule

Human-readable communication should normally remain human-readable Meshtastic traffic.

Structured NCTNL packets should be used where software must reliably interpret an event or state.
