# NCTNL

NCTNL is a Meshtastic-based communications network which extends Meshtastic rather than replacing it.

NCTNL retains normal Meshtastic LoRa communications, routing, encryption, channels, node database behaviour and messaging. NCTNL-specific functionality is added on top, with unnecessary changes to upstream Meshtastic deliberately avoided.

## Current development state

The primary NCTNL Field Node currently under development is the **ELECROW ThinkNode M5**.

Current firmware base: **Meshtastic 2.7.26**

Current NCTNL integration branch: `feature/nctnl-stage1-completion`

Current physically verified NCTNL build: **2.7.26.9165331**

## Documentation paths

If you want to use NCTNL, start with [Getting Started](Getting-Started).

If you want to understand what is currently implemented, see [Current Capabilities](Current-Capabilities).

If you want to understand how NCTNL works internally, see [System Architecture](System-Architecture), [Communications and Protocol](Communications-and-Protocol), and [Firmware Architecture](Firmware-Architecture).

If you are maintaining or upgrading the firmware, see the [Meshtastic Modification Index](Meshtastic-Modification-Index), [Development History](Development-History), and [Testing and Verification](Testing-and-Verification).

## Documentation status

This Wiki distinguishes clearly between:

- **Implemented and physically verified**
- **Implemented but not physically verified**
- **Experimental**
- **Planned**
- **Deprecated / abandoned**

Planned behaviour must not be treated as current functionality.
