# Roadmap

This page describes planned work. Items here are not current functionality unless separately marked as implemented.

## Current status

| Area | Status |
| --- | --- |
| NCTNL Settings crash | Complete and physically verified |
| Structured event foundation / NCTNL_DATA JSON v1 events on `TEXT_MESSAGE_APP` | Complete and physically verified |
| Battery state and NCTNL battery alerts | Complete, real-discharge test pending |
| Shared RTTTL alert framework | Complete (non-blocking; only Warning, BatteryLow and siren are wired in) |
| NCTNL audio alerts | Partial (battery and assistance-received done; others planned) |
| NCTNL Quiet (Standby/Offline) | Complete and physically verified |
| Quick Chat hides NCTNL_DATA / NCTNL_CTRL | Complete (Stage 4.1, pending device test) |
| Battery telemetry investigation | Complete (101% explained, curve fix planned) |
| Permanent NCTNL PortNum | Not planned for now. Data events use `TEXT_MESSAGE_APP` so all routers relay them |
| Last-known GPS persistence across reboot | Planned |
| Battery curve (OCV) correction | Planned |
| Receive-side alerts for SOS | Planned |
| FCU retransmit of unacknowledged data events (handled on the FCU, not the node) | Planned |

## Communications foundation

- common NCTNL communication layer;
- presence / heartbeat;
- last-known GPS persistence across reboot;
- battery curve (OCV) correction;
- NCTNL Ping Response; and
- airtime optimisation.

## Audio and alerts

- audio configuration;
- the remaining NCTNL audio alerts; and
- receive-side alerts for SOS.

Exact authoritative RTTTL strings should be documented only from the agreed catalogue, not recreated from memory.

## Location and privacy

- formal location/privacy framework;
- clearer policy for automatic versus manual position sharing; and
- appropriate handling of missing, stale and approximate location.

## SOS

Planned design currently includes:

- hold both ThinkNode M5 buttons for approximately three seconds;
- enter local SOS mode;
- approximately 20-second pre-transmission countdown;
- E-ink-friendly countdown behaviour;
- final Continue SOS / Cancel SOS confirmation;
- default selection of Cancel;
- fail closed if confirmation UI fails;
- no GPS fix must not prevent SOS;
- include the best available location where possible;
- configurable SOS channel;
- never blindly hard-code SOS to LongFast/channel 0;
- fail closed if the configured SOS channel is missing or disabled;
- pre-transmission cancellation does not generate `SOS_CANCEL`;
- cancelling an active transmitted SOS should eventually send `SOS_CANCEL` referencing the active SOS; and
- status should only become SOS when the SOS reaches the appropriate active/transmitted state.

## Base Station

The Base Station is being developed as a separate workstream.

The Wiki reserves space for Base Station documentation, but architecture should only be recorded once decisions are actually made.

## Quality

- hardening;
- formal regression testing;
- upgrade/rebase procedure;
- broader interoperability testing; and
- documentation completion.
