# Roadmap

This page describes planned work. Items here are not current functionality unless separately marked as implemented.

## Communications foundation

- formal NCTNL structured event protocol;
- permanent NCTNL PortNum;
- common NCTNL communication layer;
- presence / heartbeat;
- battery monitoring and events;
- NCTNL Ping Response; and
- airtime optimisation.

## Audio and alerts

- shared RTTTL/audio framework;
- audio configuration; and
- received-event alert behaviour.

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
