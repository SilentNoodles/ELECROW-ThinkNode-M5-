# Current Capabilities

This page summarises the current NCTNL ThinkNode M5 implementation.

## Implemented and physically verified

The current field-node firmware includes:

- persistent NCTNL configuration architecture;
- NCTNL module registration;
- physical ThinkNode M5 button mapping;
- NCTNL Quick Message Menu;
- reuse of the existing Meshtastic destination-selection behaviour;
- five predefined NCTNL Quick Message actions;
- Free Text;
- Send Coordinates;
- automatic NCTNL Status Message updates;
- live local status refresh after an automatic status update;
- correct post-send UI cleanup for predefined Quick Messages and Send Coordinates;
- NCTNL branding; and
- central development configuration for currently hard-coded NCTNL feature enablement.

## Predefined Quick Messages

The current actions are:

- Going Offline
- Going Standby
- Check In
- Need Assistance
- All Clear

These are transmitted as normal Meshtastic text messages, not as structured NCTNL events.

## Current structured-event foundation

A minimal `NctnlEvent` protobuf and NCTNL module packet path already exist. They currently use Meshtastic `PRIVATE_APP`.

This foundation remains for future machine-to-machine functionality. It must not be confused with the human-readable Quick Message transport.

## Not yet implemented as completed functionality

Examples of planned work include:

- a permanent NCTNL PortNum;
- formal structured event protocol;
- presence and heartbeat;
- battery events;
- ping/response;
- shared RTTTL/audio framework;
- location/privacy framework;
- SOS activation, transmission and cancellation;
- Base Station emergency handling;
- hardening and airtime optimisation; and
- formal regression testing.
