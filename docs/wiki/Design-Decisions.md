# Design Decisions

This page records important architectural choices and their rationale.

## ADR-001: Extend Meshtastic rather than replace it

**Decision:** NCTNL is built as an extension to Meshtastic.

**Reason:** Meshtastic already provides the radio network, routing, encryption, channels, nodes and messaging foundation required by NCTNL.

**Consequence:** NCTNL should minimise changes to upstream behaviour and reuse existing components where practical.

## ADR-002: Human-readable Quick Messages use normal Meshtastic text

**Decision:** The five predefined NCTNL Quick Messages use the normal text-message path.

**Reason:** Their purpose is to communicate readable information to people. Ordinary compatible Meshtastic clients should be able to receive and display them.

**Consequence:** The structured NCTNL event path is not the primary transport for these actions.

## ADR-003: Structured events are reserved for machine-to-machine functions

**Decision:** Keep the `NctnlEvent` foundation for future software-interpreted events.

**Reason:** Functions such as presence, battery state or emergency state may require consistent machine-readable semantics.

## ADR-004: Reuse Meshtastic Status Message

**Decision:** NCTNL status is represented using the existing Meshtastic Status Message feature.

**Reason:** A second parallel status system would duplicate persistence, transport and client functionality.

## ADR-005: Reuse the existing destination-selection model

**Decision:** NCTNL uses Meshtastic's existing channel/node destination model.

**Reason:** This reduces code duplication and keeps message behaviour familiar and interoperable.

## ADR-006: Do not maintain a custom Android client

**Decision:** A dedicated NCTNL fork of the Meshtastic Android application is not currently part of the design.

**Reason:** Maintaining a permanent custom mobile-client fork would significantly increase long-term maintenance.

## ADR-007: Abandon M5-hosted Web Config

**Decision:** The M5-hosted Web Config proof of concept is no longer current functionality.

**Reason:** The device could report that AP, DHCP and HTTP services were running, but physical clients were unable to communicate successfully with the service.

## ADR-008: Use central development settings temporarily

**Decision:** During active development, key NCTNL functions are enabled through a central firmware configuration header.

**Reason:** This allows development to proceed without committing to a user-interface configuration model that may later change.

## ADR-009: Minimise upstream Meshtastic modifications

**Decision:** Prefer integration points and wrappers over unnecessary rewrites.

**Reason:** Smaller upstream deltas reduce upgrade complexity and regression risk.
