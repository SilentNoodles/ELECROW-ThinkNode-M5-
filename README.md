# NCTNL Node Firmware

> **NCTNL development firmware for Meshtastic field nodes**

NCTNL Node Firmware is a customised Meshtastic firmware build developed for NCTNL field nodes. It extends Meshtastic rather than replacing it, retaining the existing LoRa mesh, routing, encryption, channels, Bluetooth, GPS, telemetry, node database and messaging systems while adding NCTNL-specific standalone-device functions.

> [!WARNING]
> NCTNL Node Firmware is currently **pre-release development firmware**. Features, configuration structures and the NCTNL machine-readable protocol may change before a stable release.

| | |
| --- | --- |
| **Current hardware target** | ELECROW ThinkNode M5 |
| **Meshtastic base** | 2.7.26 |
| **Current physically verified reference build** | `2.7.26.9165331` |
| **Reference commit** | `9165331901a87783d76b49c9af974b1b67e09a47` |
| **Build target** | `thinknode_m5` |
| **Licence** | GPL-3.0 |

## What is NCTNL?

NCTNL is being developed as a Meshtastic-based communications network with additional functions intended for field use, status reporting, assistance requests and future Base Station integration.

The firmware is designed around a simple principle: **an NCTNL node should remain a Meshtastic node**.

NCTNL therefore reuses Meshtastic's existing radio, mesh routing, encryption, channels, Bluetooth, GPS, telemetry, messaging, display, input and configuration architecture wherever practical.

The project does not introduce a replacement LoRa stack, a second mesh-routing system or parallel encryption system.

## Design Goals

- Preserve normal Meshtastic functionality wherever practical.
- Make important NCTNL field actions available directly from the node.
- Avoid requiring a custom phone application for routine NCTNL functions.
- Reuse Meshtastic routing, encryption and channel behaviour.
- Keep NCTNL-specific code isolated and minimise changes to Meshtastic core components.
- Keep ordinary messages readable by standard compatible Meshtastic clients where possible.
- Treat LoRa airtime as a limited resource.
- Preserve a path back to standard Meshtastic behaviour where practical.
- Physically test features on the target hardware before treating them as complete.

## Supported Hardware

Development is currently focused on the **ELECROW ThinkNode M5**.

| Component | Configuration |
| --- | --- |
| MCU | ESP32-S3 |
| LoRa radio | SX1262 |
| Display | 200 × 200 e-ink |
| GPS | L76K |
| Controls | Two physical buttons |
| Buzzer | GPIO-controlled buzzer |
| PlatformIO environment | `thinknode_m5` |
| Maximum configured LoRa TX power | 22 dBm |

Additional NCTNL hardware may be supported later. Unless explicitly documented, other Meshtastic devices should not be assumed to support the NCTNL-specific on-device interface.

---

## Current NCTNL Features

### NCTNL Module

The firmware contains a dedicated `NctnlModule` registered with the Meshtastic module system.

It currently provides the foundation for:

- NCTNL enable state.
- NCTNL Quick Message Menu state.
- Automatic NCTNL status updates.
- Persistent protobuf-backed NCTNL configuration.
- Machine-readable NCTNL event definitions.
- Future NCTNL protocol functions.

During active development, implemented NCTNL features are enabled through central development settings while a suitable user-accessible configuration method is developed.

### Quick Message Menu

The **NCTNL Quick Message Menu** provides common field actions directly from the ThinkNode M5.

```text
To: <selected destination>
[Select Destination]
Going Offline
Going Standby
Check In
Need Assistance
All Clear
[Send Coordinates]
[- Free Text -]
[Exit]
```

`[Select Destination]` uses the existing Meshtastic destination system. Configured channels and known individual nodes are presented through the same destination picker.

The five NCTNL actions are deliberately sent as **normal Meshtastic text messages**. They can therefore be received and read by standard compatible Meshtastic clients without requiring NCTNL-specific client software.

### Quick Actions

| Action | Status | Purpose |
| --- | --- | --- |
| **Going Offline** | `Offline` | Announces that the node is going offline and will no longer be available until it returns. |
| **Going Standby** | `Standby` | Announces that the node remains available but responses may be delayed. |
| **Check In** | `Online` | Confirms that the node is OK and no assistance is currently required. |
| **Need Assistance** | `Needs Assistance` | Requests non-emergency assistance and asks another operator to establish contact. |
| **All Clear** | `Online` | Confirms that the previous situation has been resolved. |

> [!IMPORTANT]
> **Need Assistance is not an SOS.** It is intentionally a lower-severity, non-emergency request.

### Send Coordinates

`[Send Coordinates]` sends the node's current GPS position as an ordinary Meshtastic text message to the selected destination.

```text
Coordinates: 52.123456, -2.123456
```

A current valid GPS fix is required. If no current fix is available:

- Nothing is transmitted.
- The device displays `No GPS fix`.
- Stale, zero or placeholder coordinates are not sent.

This is a deliberate manual location-sharing action. It is separate from the planned NCTNL location/privacy system and future SOS location behaviour.

### Free Text

`[- Free Text -]` retains normal Meshtastic free-text messaging and uses the currently selected destination.

NCTNL does not replace ordinary Meshtastic messaging.

---

## Automatic NCTNL Status

NCTNL can update the node's existing Meshtastic Status Message when a Quick Message action is used.

The expected format is:

```text
{Rough Location} | {Status} | {Role} | NCTNL.io
```

For example:

```text
Example Area | Online | Relay | NCTNL.io
```

Only the **Status** field is changed automatically. Rough Location and Role remain manually controlled.

| Quick action | New status |
| --- | --- |
| Going Offline | `Offline` |
| Going Standby | `Standby` |
| Check In | `Online` |
| Need Assistance | `Needs Assistance` |
| All Clear | `Online` |

Automatic modification is deliberately conservative. The firmware only changes the status when the existing Status Message already matches the expected four-part NCTNL format and ends with `NCTNL.io`.

If the existing status does not match the expected format, it is left untouched.

A status-update failure does not prevent the selected Quick Message from being transmitted.

---

## ThinkNode M5 Interface

### Display

The ThinkNode M5 is configured as a **200 × 200 e-ink BaseUI device**.

The available screen carousel depends on device configuration, enabled modules, GPS availability and favourite nodes. The M5 build can include:

| Screen | Purpose |
| --- | --- |
| Home | Local node/device information |
| Messages | Meshtastic text messages |
| Last Heard | When other nodes were last heard |
| Hop / Signal | Hop and signal information |
| Distance | Distance information for known nodes |
| Bearings | Directional information when GPS data is available |
| GPS / Compass | Local GPS and location information |
| LoRa | Radio information |
| System | Device/system information |
| Clock | Digital or analogue clock |
| Chirpy | Meshtastic Chirpy frame |
| Wi-Fi | Conditional Wi-Fi information |
| Module frames | Frames supplied by enabled Meshtastic modules |
| Favourite nodes | Individual frames for favourited nodes |

A Critical Fault frame can also be inserted when the firmware reports an active critical error.

### Physical Controls

| Control | GPIO | Function |
| --- | ---: | --- |
| Primary / upper button | 21 | Primary / User button |
| Alternate / lower button | 14 | Alternate / Back button |

The M5 continues to use Meshtastic's existing input architecture rather than having the NCTNL module directly poll the button GPIOs.

The planned dual-button NCTNL SOS gesture is **not implemented yet**.

---

## Configuration

NCTNL configuration fields use Meshtastic's protobuf-backed configuration architecture.

Current development settings include:

```text
NCTNL Enabled: Enabled
Quick Message Menu: Enabled
Automatic Status Updates: Enabled
```

For the current development firmware these effective values are centrally enabled. The persistent configuration fields remain in place so a future configuration interface can use them without redesigning the feature logic.

### NCTNL Settings

A read-only **NCTNL Settings** entry currently exists in the ThinkNode M5 Home Action menu.

> [!CAUTION]
> In the current reference build, selecting NCTNL Settings can cause the device to abort and reboot. The fault has been traced to the settings banner callback path and is awaiting a firmware fix.

The current Settings page should therefore not be relied upon.

---

## NCTNL Event Protocol

The repository contains the initial machine-readable `NctnlEvent` protobuf foundation.

Current event types are:

```text
UNKNOWN
GOING_OFFLINE
STANDBY
CHECK_IN
ASSISTANCE
ALL_CLEAR
```

The initial structure contains:

- Protocol version.
- Event type.
- 32-bit event ID.

This is an **experimental protocol foundation**, not the final NCTNL wire protocol.

The five current user-facing Quick Message actions use ordinary Meshtastic text messages rather than depending on structured NCTNL packets.

A permanent NCTNL PortNum and the wider structured protocol are intentionally deferred until the Base Station and protocol architecture are ready.

---

## Current Reference Build

The current physically verified firmware reference is:

```text
NCTNL Stage 3B
S3B-09 - Status Display and Coordinates UI Fix

Firmware: 2.7.26.9165331
Commit: 9165331901a87783d76b49c9af974b1b67e09a47
```

Physical testing has confirmed:

- NCTNL module startup.
- Quick Message Menu operation.
- Destination selection.
- Transmission of all five Quick Message actions as normal Meshtastic text.
- Free-text transmission.
- Manual Send Coordinates behaviour.
- Automatic NCTNL Status Message updates.
- Live visibility of the updated status on the node display.
- Correct UI cleanup after sending Quick Messages.
- Correct UI cleanup after sending coordinates.

A successful compile alone is not considered physical verification.

---

## Known Issues

### NCTNL Settings Reboot

Selecting `NCTNL Settings` from the Home Action menu currently causes an abort/reboot.

The captured backtrace identifies an empty `std::function` invocation in the overlay-banner callback path. A minimal fix is pending.

### Battery Telemetry Can Report 101%

While charging, the battery percentage shown locally on the ThinkNode M5 correctly tops out at 100%, but telemetry presented to a Meshtastic client can report **101%**.

This affects battery graphs and derived statistics. The battery value path through Meshtastic `DeviceMetrics` telemetry is due to be investigated.

### Boot-Time Invalid GPIO Message

Development logs have also shown an `Invalid pin selected` message during boot. This is tracked separately and is not currently known to prevent normal LoRa, Bluetooth or NCTNL operation.

---

## Roadmap

| Area | Status |
| --- | --- |
| Persistent NCTNL configuration foundation | Complete |
| ThinkNode M5 physical button mapping | Complete |
| NCTNL Quick Message Menu | Complete |
| Quick Message text transmission | Complete and physically verified |
| Manual Send Coordinates | Complete and physically verified |
| Automatic NCTNL status updates | Complete and physically verified |
| NCTNL branding | Complete |
| Structured event foundation | Experimental foundation present |
| NCTNL Settings crash | Fix pending |
| Battery telemetry investigation | Planned |
| Presence / heartbeat | Planned |
| Battery state and NCTNL battery alerts | Planned |
| Shared RTTTL alert framework | Planned |
| NCTNL audio alerts | Planned |
| Location and privacy controls | Planned |
| Local SOS activation and UI | Planned |
| SOS transmission and cancellation | Planned after protocol/Base Station design |
| Permanent NCTNL PortNum | Deferred until protocol design |
| Base Station integration | In development separately |
| Hardening and regression testing | Future |

Planned features may change before implementation.

---

## SOS

> [!WARNING]
> **SOS transmission is not implemented in the current firmware.**

The intended SOS design is being developed separately and is expected to include deliberate dual-button activation, a local countdown and confirmation step, channel validation, location handling and explicit cancellation behaviour.

Until the feature is implemented and physically verified, the ThinkNode M5 should **not** be treated as providing an NCTNL SOS function.

---

## Building

NCTNL Node Firmware uses the existing Meshtastic PlatformIO build system.

With PlatformIO CLI available:

```bash
pio run -e thinknode_m5
```

The build environment is:

```text
thinknode_m5
```

Compiled output is normally written beneath:

```text
.pio/build/thinknode_m5/
```

Always verify that firmware intended for an NCTNL ThinkNode M5 was built using the `thinknode_m5` environment.

## Flashing

The ThinkNode M5 can be flashed using the normal ESP32-S3/Meshtastic-compatible firmware update process.

For routine development updates, use the normal firmware `.bin`. Factory images should be reserved for full re-flashing or recovery where appropriate.

---

## Repository Structure

Important NCTNL-specific areas currently include:

```text
src/modules/NctnlModule.cpp
src/modules/NctnlModule.h
src/modules/NctnlDevelopmentConfig.h

protobufs/meshtastic/nctnl.proto
src/mesh/generated/meshtastic/nctnl.pb.cpp
src/mesh/generated/meshtastic/nctnl.pb.h

variants/esp32s3/ELECROW-ThinkNode-M5/
```

NCTNL functionality also integrates with existing Meshtastic components where appropriate, particularly the Canned Message/Quick Chat UI, Home Action menu, Status Message support, input system and protobuf configuration.

---

## Development Approach

NCTNL firmware is developed incrementally.

The normal process for meaningful firmware changes is:

1. Investigate the closest existing Meshtastic implementation.
2. Make the smallest practical NCTNL-specific or upstream-style change.
3. Compile the `thinknode_m5` target.
4. Archive the successful build.
5. Flash and physically test the ThinkNode M5.
6. Confirm expected behaviour before treating the feature as complete.

This keeps NCTNL close to upstream Meshtastic behaviour and reduces unnecessary divergence.

---

## Meshtastic

NCTNL Node Firmware is based on the open-source [Meshtastic Firmware](https://github.com/meshtastic/firmware) project.

For general Meshtastic documentation, see [meshtastic.org](https://meshtastic.org/).

This repository is an NCTNL-modified firmware project and is **not the official Meshtastic firmware distribution**. NCTNL-specific changes, experimental features and issues should not automatically be attributed to the upstream Meshtastic project.

## NCTNL

Project site: [NCTNL.io](https://nctnl.io/)

## Licence

This repository is distributed under the **GNU General Public License v3.0 (GPL-3.0)**.

See [LICENSE](LICENSE) for the complete licence terms.

---

> NCTNL Node Firmware is under active development. Do not assume that planned emergency, alerting, location, presence or Base Station features are available unless they are explicitly documented as implemented and physically verified.
