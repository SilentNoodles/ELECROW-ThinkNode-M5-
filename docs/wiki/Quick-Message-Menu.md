# Quick Message Menu

**Status: Implemented and physically verified**

The NCTNL Quick Message Menu on the ThinkNode M5 provides predefined operational messages, Free Text and Send Coordinates.

## Destination selection

NCTNL reuses the existing Meshtastic destination-selection flow.

Messages can therefore be directed to the selected channel or an individual node according to the normal Meshtastic message path.

The picker hides the `NCTNL_DATA` and `NCTNL_CTRL` channels (exact, case-sensitive name match) while NCTNL is enabled. `NCTNL_COMMS`, LongFast and other channels, and direct-message nodes, still appear.

## Predefined messages

### Going Offline

```text
NCTNL STATUS: Going Offline
This node is going offline and will no longer be available for communications until it returns online.
```

Automatic status mapping: **Offline**

### Going Standby

```text
NCTNL STATUS: Going Standby
This node is entering standby. Communications remain available, but responses may be delayed.
```

Automatic status mapping: **Standby**

### Check In

```text
NCTNL CHECK-IN: Status Confirmed
This node has checked in successfully. Everything is OK and no assistance is currently required.
```

Automatic status mapping: **Online**

### Need Assistance

```text
NCTNL ASSISTANCE: Assistance Requested
Non-emergency assistance has been requested. Please respond when available to establish contact and determine what assistance is required.
```

Automatic status mapping: **Needs Assistance**

### All Clear

```text
NCTNL STATUS: All Clear
The previous situation has been resolved. No further assistance is currently required.
```

Automatic status mapping: **Online**

## Transport decision

These five actions use the normal Meshtastic `TEXT_MESSAGE_APP` path.

Their primary purpose is to communicate readable information to people. Each status action also sends a separate machine-readable NCTNL_DATA `sts` event on the `NCTNL_DATA` channel.
