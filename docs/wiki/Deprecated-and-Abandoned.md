# Deprecated and Abandoned

This section records work which should not be mistaken for current NCTNL functionality.

## M5-hosted Web Config

**Status: Deprecated / abandoned**

A local configuration service was developed for the ThinkNode M5, including a temporary Wi-Fi access point, DHCP/HTTP lifecycle and NCTNL configuration handlers.

Although the firmware could report the services as running, real client devices could not communicate successfully with the service during physical testing.

The feature was abandoned.

### Why retain the history?

The experiment identified the cost and fragility of making NCTNL compete with Meshtastic's existing networking lifecycle. Future configuration work should avoid repeating the same architecture without a clear reason.

## Custom Android configuration

**Status: Rejected design approach**

Maintaining a custom Meshtastic Android application was considered undesirable because it would introduce a substantial permanent maintenance burden.

## Structured transport for human-facing Quick Messages

**Status: Superseded**

The original structured `PRIVATE_APP` transport for the five predefined Quick Messages was replaced with ordinary Meshtastic text messages.

The structured event framework itself remains present for future machine-to-machine use.

## Temporary button diagnostics

**Status: Removed after verification**

Diagnostic instrumentation used to verify the ThinkNode M5 physical controls was deliberately removed once the hardware mapping had been physically confirmed.
