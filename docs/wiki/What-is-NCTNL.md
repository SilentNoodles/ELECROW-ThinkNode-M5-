# What is NCTNL?

NCTNL is a communications project built on Meshtastic.

It is not intended to replace Meshtastic. Instead, it keeps the normal Meshtastic network and adds NCTNL-specific behaviour where useful.

## Core principles

NCTNL is designed to:

- retain normal Meshtastic LoRa communications;
- retain Meshtastic routing and encryption;
- retain Meshtastic channels and node database behaviour;
- retain normal Meshtastic messaging;
- add NCTNL-specific functionality on top;
- keep NCTNL-specific behaviour isolated where practical;
- remain interoperable with ordinary Meshtastic nodes;
- minimise unnecessary LoRa airtime;
- use normal human-readable Meshtastic messages where appropriate; and
- reserve structured NCTNL packets for functions that genuinely require machine-to-machine communication.

## Intended node roles

At minimum, the project is expected to support:

- **Field Node**
- **Relay Node**
- **Base Station**

Only the Field Node is substantially implemented at present. Relay Node and Base Station behaviour should not be inferred from the names alone.

## Why build on Meshtastic?

Meshtastic already provides a mature encrypted LoRa mesh, routing, channels, node discovery and messaging model. Reusing those capabilities means NCTNL can focus on additional operational functions rather than duplicating an existing mesh stack.
