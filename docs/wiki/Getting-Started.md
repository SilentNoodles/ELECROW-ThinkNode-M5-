# Getting Started

This page will become the operational starting point for preparing an NCTNL Field Node.

## Current supported development hardware

The primary development target is:

**ELECROW ThinkNode M5**

Other hardware should not be assumed to support the NCTNL firmware unless it has been specifically documented and tested.

## Current firmware reference

- Meshtastic base: **2.7.26**
- Integration branch: `feature/nctnl-stage1-completion`
- Physically verified build: **2.7.26.9165331**

## Initial setup principles

NCTNL continues to use normal Meshtastic configuration for radio operation, channels, encryption, node identity and ordinary messaging.

NCTNL-specific development features are presently controlled centrally in firmware rather than through a user-editable NCTNL settings interface.

The NCTNL settings/status page on the ThinkNode M5 is currently informational and read-only.

## Before using operational features

Verify:

1. the device is running the intended NCTNL build;
2. normal Meshtastic messaging works;
3. the intended channel configuration is present;
4. the NCTNL Quick Message Menu opens;
5. the destination shown before sending is correct;
6. GPS has a valid current fix before using Send Coordinates; and
7. the Status Message is in the expected NCTNL format if automatic status updates are required.

More detailed flashing and build instructions will be added as the development workflow is standardised.
