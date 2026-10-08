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
- Online status at every start-up;
- Offline status on battery Emergency, restored on the charging all-clear;
- NCTNL Quiet: broadcast message notifications are skipped while the status is Offline or Standby (direct messages still alert; Meshtastic Mute is unchanged);
- correct post-send UI cleanup for predefined Quick Messages and Send Coordinates;
- NCTNL_DATA JSON v1 events (`sts`, `bat`) sent as `TEXT_MESSAGE_APP` on the `NCTNL_DATA` channel;
- read-only NCTNL Settings page (the earlier crash is fixed);
- NCTNL branding; and
- central development configuration for currently hard-coded NCTNL feature enablement.

## Implemented, not yet physically verified

- battery low/critical/emergency alerts, the Emergency → Offline change and the charging restore (real-discharge test pending);
- non-blocking RTTTL alert tones (only Warning, BatteryLow and siren are wired in); and
- the Quick Chat destination picker hides `NCTNL_DATA` and `NCTNL_CTRL`.

## Predefined Quick Messages

The current actions are:

- Going Offline
- Going Standby
- Check In
- Need Assistance
- All Clear

These are transmitted as normal Meshtastic text messages. Each status action also sends a matching NCTNL_DATA `sts` event.

## Current structured-event foundation

Machine-readable events are compact JSON sent as `TEXT_MESSAGE_APP` on the `NCTNL_DATA` channel, so all routers relay them. See `docs/nctnl-data-spec.md`. NCTNL_DATA text received by the node is not stored or shown as a message; an assistance request shows an alert banner instead.

## Not yet implemented as completed functionality

Examples of planned work include:

- presence and heartbeat;
- ping/response;
- the remaining NCTNL audio alerts;
- battery curve (OCV) correction;
- last-known GPS persistence across reboot;
- location/privacy framework;
- SOS activation, transmission and cancellation;
- Base Station emergency handling;
- hardening and airtime optimisation; and
- formal regression testing.
