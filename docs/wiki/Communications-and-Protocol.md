# Communications and Protocol

NCTNL currently uses two distinct communication concepts.

## 1. Human-readable Meshtastic messages

The five predefined Quick Messages, Free Text and Send Coordinates use normal Meshtastic text messaging.

This preserves compatibility with ordinary Meshtastic clients and avoids hiding human-facing information inside a custom application packet.

## 2. Structured NCTNL events

A structured `NctnlEvent` protobuf foundation exists in the firmware.

The current NCTNL module is a `SinglePortModule` using Meshtastic `PRIVATE_APP`.

The current event structure includes a protocol version, event type and event ID.

The structured path is retained for future functionality requiring machine interpretation.

## Current protocol maturity

There is **not yet a permanent NCTNL Meshtastic PortNum**.

The existing structured-event mechanism should therefore be treated as an experimental foundation, not a final public NCTNL wire protocol.

## Planned event categories

The project has considered event types such as:

- ONLINE
- HEARTBEAT
- GOING_OFFLINE
- STANDBY
- CHECK_IN
- ASSISTANCE
- ALL_CLEAR
- BATTERY_LOW
- BATTERY_CRITICAL
- SOS
- SOS_CANCEL

Their presence in planning or protobuf work must not be interpreted as evidence that every event is currently implemented operationally.

## Airtime

LoRa airtime is a constrained shared resource.

NCTNL therefore aims to avoid adding periodic or duplicate transmissions unless they provide clear operational value.
