# NCTNL FCU Serial Mirror — Line Format v1.1

The FCU serial mirror lets an attached FCU (for example a Raspberry Pi on the node's USB serial port) log everything the node sends, receives and exchanges with its phone, plus node, battery and GPS status, without staying connected as a Meshtastic API client. The phone stays connected over Bluetooth as normal.

The mirror only observes. It never touches the toPhone queue, the MeshService queues or any packet flow; it writes extra text lines to the serial console. Since v1.1 the FCU can also send a few plain-text [command lines](#c-fcu-command-lines) back to the node.

## Changes in v1.1

- The console [leaves API mode](#serial-api-sessions-and-sync) after a serial API session (ToRadio `disconnect`, or 30 s without a frame). The mirror then resumes with an `H` line and a [replay](#replay-after-a-sync).
- [Hello v2](#h-hello): `"v":2` with `ln`, `sn` and `gps`; every 15 s for the first minute of uptime.
- [`C` command lines](#c-fcu-command-lines) from the FCU: `sync`, `fcu` (keep-alive) and `gps`.
- New node lines: [`K`](#k-command-response) (command response), [`G`](#g-local-gps) (local GPS), [`M`](#m-chat-replay) (chat replay) and [`D`](#d-nctnl-text-replay) (NCTNL_DATA / NCTNL_CTRL replay, M5 only).
- An "FCU sync..." [banner](#sync-banner) during a serial API session.
- `NCTNL_CTRL` text is now [hidden](#hidden-nctnl_ctrl-text) like `NCTNL_DATA` (M5 only).

Everything else in v1 is unchanged: the `@NCTNL1` tag, `seq`, CRC, base64 payloads, the leading `\r\n`, the drop rule, the lock order and ADMIN payload stripping.

## Availability

| Gate            | Value                                                                                                                                                        |
| --------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Build flag      | `NCTNL_FCU_MIRROR=1`, set for `thinknode_m5` and `thinknode_m2`. Without it none of the mirror code is compiled and the console behaves as stock Meshtastic. |
| M5 only         | `D` lines, the NCTNL text store and hidden `NCTNL_CTRL` text (they need the M5 NCTNL hooks). Everything else applies to both boards.                         |
| GPS             | `G` lines, `"gps":1` and the `gps` command need a GPS that is present and enabled. The M2 has no GPS, so it never writes `G` lines.                          |
| Runtime setting | `moduleConfig.nctnl.fcu_integration_disabled` (`NctnlConfig` field 3). `false`/unset = mirror **on**. When off, command lines are ignored too.               |
| API mode        | No mirror lines and no commands while a Meshtastic serial API client is using the console (see [API sessions](#serial-api-sessions-and-sync)).               |
| Independent of  | Debug logging, serial log levels, `debug_log_api_enabled`, `moduleConfig.nctnl.enabled`.                                                                     |

The setting is inverted on purpose: devices that already have a saved config read the new field as `false`, so the mirror is on after the update. There is no on-device toggle yet; on the M5 the current state is shown as `FCU:On`/`FCU:Off` on the NCTNL Settings status page (the M2 has no such page).

## Line format

```text
\r\n@NCTNL1 <type> <seq> [<extra> ...] <payload> *<crc>\r\n
```

| Field       | Meaning                                                                                                                                                  |
| ----------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `\r\n`      | Every line is preceded by a line break so it always starts on a fresh line, even if a log line was cut short. It is not part of the CRC.                 |
| `@NCTNL1`   | Fixed tag. The `1` is the format version; v1.1 is backwards compatible and keeps it.                                                                     |
| `<type>`    | One ASCII letter, see [Types](#types).                                                                                                                   |
| `<seq>`     | Decimal `uint32`. Starts at `0` at boot and increases by 1 for every line the node tries to write, including dropped lines. A gap means lines were lost. |
| `<extra>`   | Zero or more type-specific fields (decimal or short tokens). Only `T`, `E` and `K` have one.                                                             |
| `<payload>` | RFC 4648 base64 **with** `=` padding, for every type.                                                                                                    |
| `<crc>`     | 8 lowercase hex digits: CRC-32 (IEEE 802.3, identical to Python `zlib.crc32`) over every byte from `@` up to and including the space before `*`.         |

Fields are separated by exactly one space and lines end with `\r\n`. The base64 alphabet contains no spaces, so the payload is always the last field before ` *`. Every mirror byte is 7-bit ASCII, so a line can never contain the Meshtastic stream framing byte `0x94`.

The CRC is the standard reflected CRC-32: polynomial `0xEDB88320`, initial value `0xFFFFFFFF`, final XOR `0xFFFFFFFF`. Check value: `crc32("123456789") = cbf43926`.

All JSON payloads are compact UTF-8 JSON. Strings are escaped with `\"`, `\\` and `\u00XX` for control characters; other bytes (including multi-byte UTF-8 such as emoji) are copied unchanged.

## Types

| Type | Extra fields   | When                                                                                                    | Payload (after base64 decoding)                                                              |
| ---- | -------------- | ------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------- |
| `H`  | —              | Hello, see [schedule](#h-hello). Also immediately after every serial API session.                       | JSON, see [Hello](#h-hello).                                                                 |
| `B`  | —              | Battery: every 60 s, and within about 1 s of a change in USB power or charging state.                   | JSON, see [Battery](#b-battery).                                                             |
| `G`  | —              | Local GPS state, see [G](#g-local-gps).                                                                 | JSON.                                                                                        |
| `K`  | `<cmd>`        | Response to a `C` command. Only `sync` has one.                                                         | JSON, see [K](#k-command-response).                                                          |
| `M`  | —              | Chat history replay after a serial API session.                                                         | JSON, see [M](#m-chat-replay).                                                               |
| `D`  | —              | NCTNL_DATA / NCTNL_CTRL replay after a serial API session (M5 only).                                    | JSON, see [D](#d-nctnl-text-replay).                                                         |
| `R`  | —              | Every packet received over LoRa, at the point the firmware logs `Lora RX` (before duplicate filtering). | `meshtastic.MeshPacket` as received: usually `encrypted`, with `rx_*`, hop and relay fields. |
| `T`  | `<airtime_ms>` | Every packet transmitted, at the point the firmware logs `Completed sending`. Includes relays.          | `meshtastic.MeshPacket` as transmitted (on-air, usually `encrypted`).                        |
| `P`  | —              | Every packet passed to `MeshService::sendToPhone()`, before it is queued for the phone.                 | `meshtastic.MeshPacket`, decoded where the node could decrypt it.                            |
| `U`  | —              | Every `MeshPacket` the phone sends (`MeshService::handleToRadio()`).                                    | `meshtastic.MeshPacket`, decoded. `from` is set to this node's number in the mirrored copy.  |
| `E`  | `<code>`       | A line could not be built.                                                                              | Short ASCII reason, starting with the type letter of the line that failed.                   |

`T`'s `<airtime_ms>` is the time on air the firmware computed for the packet, the same value as the `Packet TX: <n>ms` debug log line.

`P` fires for every packet handed to the phone path, whether or not a phone is connected and whether or not the to-phone queue has room; the mirror is written before the queue is touched.

`E` codes: `enc` (the `MeshPacket` could not be protobuf-encoded) and `len` (the JSON or the line would not fit in its buffer). An `E` line replaces the line that failed and uses its sequence number.

**ADMIN_APP:** in `P` and `U`, packets whose decoded `portnum` is `ADMIN_APP` are mirrored with `decoded.payload` emptied, because admin payloads can carry keys and configuration. All header fields (`from`, `to`, `id`, `channel`, `want_response`, and so on) are kept. `R`/`T` copies are mirrored as they are, since on air they are encrypted.

A packet can appear several times with different types, for example an incoming text message appears as `R` (encrypted, from the radio) and then `P` (decoded, to the phone). Use `id` and `from` to correlate them.

### H, hello

```text
{"v":2,"node":1,"hw":"THINKNODE_M5","fw":"2.7.26.90070ca","nb":"90070ca","up":5,"boots":12,"ch":["","Ops","","","","","",""],"drop":0,"fcu":1,"ln":"Test \"Node\" \\ ✓ 🚀","sn":"🚀","gps":1}
```

| Key     | Meaning                                                                                                               |
| ------- | --------------------------------------------------------------------------------------------------------------------- |
| `v`     | Hello format version: `2` from v1.1 (v1 firmware sent `1` without the last three keys).                               |
| `node`  | This node's number, decimal (`1` = `!00000001`).                                                                      |
| `hw`    | Hardware model name, e.g. `THINKNODE_M5` or `THINKNODE_M2` (the `meshtastic.HardwareModel` enum name).                |
| `fw`    | Firmware `APP_VERSION`.                                                                                               |
| `nb`    | Build label: the git short hash from `APP_VERSION`, or the PlatformIO env name if the version has no hash.            |
| `up`    | Seconds since boot (does not wrap at 49.7 days).                                                                      |
| `boots` | Boot counter (`myNodeInfo.reboot_count`).                                                                             |
| `ch`    | Channel names by slot index, 8 entries. `""` for a channel that uses the default (preset) name or for an unused slot. |
| `drop`  | Number of mirror lines dropped since boot because the console was busy.                                               |
| `fcu`   | `1` when the FCU mirror is enabled. Hello lines are only written while it is enabled, so this is always `1`.          |
| `ln`    | Owner long name (up to 39 bytes of UTF-8).                                                                            |
| `sn`    | Owner short name (up to 4 bytes of UTF-8).                                                                            |
| `gps`   | `1` when a GPS is present (it has sent valid NMEA) and enabled (`gps_mode` `ENABLED`), else `0`.                      |

Keys are always in this order; new keys are only ever appended. Decoders should ignore keys they don't know.

Schedule: the first hello as soon as the node number is known, then every 15 s while uptime is under 60 s, then every 60 s. An extra hello is written as soon as a serial API session ends.

The hello never contains keys (no PSKs, no public or private keys). The JSON buffer is 1024 bytes; the worst case (every byte of a 39-byte long name, a 4-byte short name and eight 11-byte channel names being a control character, so each takes 6 bytes) is 1002 bytes, and maximum-length UTF-8 names are far shorter because UTF-8 bytes are not escaped.

### B, battery

```text
{"mv":4012,"pct":87,"usb":1,"chg":1}
```

| Key   | Meaning                                                                                                     |
| ----- | ----------------------------------------------------------------------------------------------------------- |
| `mv`  | Battery voltage in mV. `-1` until the power subsystem has a real reading, and while no battery is detected. |
| `pct` | Battery state of charge, 0–100, estimated from the voltage. Never 101. `-1` whenever `mv` is `-1`.          |
| `usb` | `1` when USB/external power is present.                                                                     |
| `chg` | `1` when charging.                                                                                          |

The first `B` at boot is written before the first power reading, so it carries `-1` for `mv` and `pct`.

### G, local GPS

The node's own GPS state, from data the node already holds. `G` lines are only written to the USB serial console: nothing is transmitted, position broadcast settings are not changed and nothing is written to flash.

With a fix:

```text
{"fix":1,"lat":123456789,"lon":-123456789,"alt":42,"sats":9,"hdop":87,"spd":139,"trk":27000000,"t":1700000000,"age":3}
```

Without a fix:

```text
{"fix":0,"sats":2,"age":-1}
```

| Key    | Meaning                                                                                                                         |
| ------ | ------------------------------------------------------------------------------------------------------------------------------- |
| `fix`  | `1` while the node holds a valid GPS location, else `0`.                                                                        |
| `lat`  | `latitude_i`, 1e-7 degrees (`123456789` = 12.3456789).                                                                          |
| `lon`  | `longitude_i`, 1e-7 degrees.                                                                                                    |
| `alt`  | Altitude above MSL in metres.                                                                                                   |
| `sats` | Satellites in view.                                                                                                             |
| `hdop` | HDOP × 100, as stored by the firmware (`87` = 0.87).                                                                            |
| `spd`  | Ground speed in cm/s. The firmware stores whole km/h, so the resolution is about 28 cm/s.                                       |
| `trk`  | Ground track (course over ground) in 1e-5 degrees (`27000000` = 270°).                                                          |
| `t`    | UTC time of the fix from the GPS, epoch seconds.                                                                                |
| `age`  | Seconds since the node last saw a new fix (`t` changed). Without a fix: seconds since the last fix, or `-1` if none since boot. |

When:

- every 30 s while the FCU is [present](#fcu-keep-alive), otherwise every 120 s;
- immediately when the fix is gained or lost;
- in answer to the [`gps`](#gps-fresh-fix) command.

Between GPS search cycles the GPS may sleep while the node still holds its last fix; the line then has `"fix":1` with a growing `age`.

### K, command response

```text
@NCTNL1 K <seq> <cmd> <payload> *<crc>
```

`<cmd>` is the command being answered. Only `sync` has a `K` line.

| Payload                                   | Meaning                                                                                       |
| ----------------------------------------- | --------------------------------------------------------------------------------------------- |
| `{"ok":1}`                                | Sync accepted. Start the API session within 30 s.                                             |
| `{"ok":0,"why":"too_soon","retry_s":412}` | Refused: the last accepted sync was less than 10 minutes ago. `retry_s` is when to ask again. |

### M, chat replay

One line per message in the node's chat history (the existing MessageStore, 20 messages), oldest first.

```text
{"r":1,"from":2,"to":4294967295,"ch":1,"t":1700000100,"bt":0,"txt":"hello \"ops\"\nline 2 👍"}
```

| Key    | Meaning                                                                                         |
| ------ | ----------------------------------------------------------------------------------------------- |
| `r`    | Always `1`: a replayed record, not a live event.                                                |
| `from` | Sender node number.                                                                             |
| `to`   | Destination node number (`4294967295` = broadcast).                                             |
| `ch`   | Channel slot index.                                                                             |
| `t`    | Timestamp as stored: epoch seconds, or seconds since that boot when `bt` is `1`.                |
| `bt`   | `1` if `t` is boot-relative (the node had no valid time when the message was stored), else `0`. |
| `txt`  | Message text.                                                                                   |

The chat history stores no packet ID, so `M` has none. Text on `NCTNL_DATA` and `NCTNL_CTRL` is never in the chat history.

### D, NCTNL text replay

M5 only. One line per entry in the hidden NCTNL text store, oldest first.

```text
{"r":1,"from":2,"to":4294967295,"ch":2,"id":305419896,"t":1700000200,"txt":"{\"k\":\"sts\",\"v\":1,\"type\":\"online\",\"id\":\"00000002-12345678\",\"node\":\"!00000002\"}"}
```

| Key    | Meaning                                                                                    |
| ------ | ------------------------------------------------------------------------------------------ |
| `r`    | Always `1`.                                                                                |
| `from` | Sender node number (this node's number for text it sent).                                  |
| `to`   | Destination (always broadcast: only broadcast text on these channels is stored).           |
| `ch`   | Channel slot index.                                                                        |
| `id`   | Packet ID.                                                                                 |
| `t`    | Epoch seconds: `rx_time` for received text, the node's clock for sent text. `0` = unknown. |
| `txt`  | Text, up to 233 bytes.                                                                     |

The store keeps the last 50 text events on the channels named `NCTNL_DATA` and `NCTNL_CTRL` (matched by name, case-insensitive, never falling back to the primary channel), both received and sent by this node (from the phone or from the NCTNL module). It is never displayed.

| Store | Size                                                                                                   |
| ----- | ------------------------------------------------------------------------------------------------------ |
| RAM   | 50 fixed entries of 252 bytes: 12,600 bytes, allocated statically.                                     |
| Flash | `/nctnl_text.dat`: 2-byte header, then per entry an 18-byte header and the text. At most 12,552 bytes. |

It is saved and loaded with the chat history, on exactly the same triggers: the MessageStore autosave timer (every 2 hours, only if the store changed), the shutdown and reboot paths that save the chat history, and the load at screen start-up. Nothing else writes it. Text received since the last save is lost on a power cut, just like chat messages.

## C, FCU command lines

The FCU can send plain-text lines to the node on the same serial port:

```text
@NCTNL1 C <cmd>\r\n
```

- Terminated by `\r`, `\n` or both. At most 48 characters before the terminator; longer lines are discarded.
- `<cmd>` is one word with no spaces. Unknown commands are ignored silently.
- Lines are read from the bytes the Meshtastic stream parser discards while it looks for the `0x94 0xC3` frame start, so they never switch the console to API mode and never disturb framing. A `0x94` byte starts a new line (the frame takes over); any other non-printable byte spoils the line up to its terminator.
- Ignored while a serial API client is using the console, and while the mirror is disabled.
- Keep-alives are not logged. The node logs only state changes: FCU present/absent and sync accepted/refused.
- No CRC: a corrupted command is either unknown (ignored) or harmless.

| Command | Purpose                            | Response    |
| ------- | ---------------------------------- | ----------- |
| `sync`  | Ask to start a serial API session. | `K` line.   |
| `fcu`   | Keep-alive; send it every 120 s.   | None.       |
| `gps`   | Ask for a fresh GPS fix.           | A `G` line. |

### sync

- Accepted if it is the first `sync` since boot, or the last accepted one was at least 10 minutes ago. Otherwise refused with `retry_s`.
- On accept the node shows the [sync banner](#sync-banner) and waits up to 30 s for the first API frame. If none arrives, the banner is cleared.
- The node cannot stop a stock API client from connecting without asking: any serial API session starts the same banner and ends with the same `H` line and replay.

### fcu, keep-alive

The FCU counts as present while a keep-alive arrived in the last 300 s. While present:

- `G` lines are written every 30 s instead of every 120 s;
- the GPS update interval is capped at 30 s as a RAM-only override (`config.position` is not changed or saved, so nothing is written to flash). A configured interval shorter than 30 s is kept. After 300 s without a keep-alive the stock interval applies again.

### gps, fresh fix

- If the GPS is sleeping between search cycles it is woken now for a fresh search.
- A `G` line is written as soon as a new fix is available (a fix whose time differs from the one the node held when the command arrived), or a `"fix":0` line after 30 s without one.
- Repeats within 5 s are ignored. No API mode, no Bluetooth effect, nothing transmitted. Ignored if the GPS is absent or disabled.

## Serial API sessions and sync

While a Meshtastic serial API client (for example the Python CLI or a phone app over USB) is using the console, the console is in protobuf mode: the mirror writes nothing and command lines are ignored. The console enters this mode on the first valid API frame.

From v1.1 it leaves protobuf mode again when either:

1. the client sends a ToRadio `disconnect` (a **normal** end); or
2. no ToRadio frame has arrived for 30 s (an **idle** end). This also covers clients that sent frames but never `want_config`.

In mirror builds the serial connection timeout is 30 s instead of the stock 15 minutes, and the session is closed properly so the phone API state resets. A client that stays connected must send a frame at least every 30 s (the Meshtastic Python library's own heartbeat is every 300 s).

On return to text mode:

1. debug log text resumes;
2. the mirror resumes;
3. an `H` line is written immediately;
4. then the [replay](#replay-after-a-sync).

`seq` continues from where it stopped; lines suppressed during API mode use no sequence number.

**Bluetooth:** stock Meshtastic turns Bluetooth off while a serial API client has requested config, and back on when the session ends. This is unchanged, so expect the phone to drop off Bluetooth during a sync and reconnect afterwards.

### Sync banner

From the accepted `sync` (or the first API frame, whichever is first) until the session ends, the screen shows `FCU sync...`.

| End                         | Screen                                               |
| --------------------------- | ---------------------------------------------------- |
| Normal (ToRadio disconnect) | `FCU sync complete` for 3 s, then the normal screen. |
| Idle (30 s timeout)         | The banner is cleared.                               |
| No session after `sync`     | The banner is cleared after 30 s.                    |

- Only serial console sessions show it, never Bluetooth ones.
- An active NCTNL battery or assistance alert banner takes priority: the sync banner is not shown over it, and if an alert replaces the sync banner, the end of the session leaves the alert alone.
- Each sync costs at most three e-ink refreshes (banner, complete, normal screen), all fast refreshes. The banner is not redrawn while the session runs. It expires on its own after 10 minutes as a safety net.
- The M2's OLED shows the same banner.

### Replay after a sync

After every serial API session, once the `H` line is written, the node replays its stores:

1. `M` lines: the chat history, oldest first;
2. `D` lines (M5 only): the NCTNL text store, oldest first.

Replay lines use normal sequence numbers. A replay is cancelled if another API session starts, and runs again from the start when that one ends.

**Pacing:** replay lines are written from the mirror's own thread without blocking. A replay line is written only when the console transmit buffer has room for the whole line plus 640 bytes kept free for live lines (`R`/`T`/`P`/`U` are at most about 560 bytes); otherwise the thread retries every 20 ms without using a sequence number. At 115200 baud that is about 11.5 kB/s, so a full 70-line replay takes a few seconds. If a replay line still has no room after 2 s, it falls back to the normal drop rule (dropped and counted), so a stuck port cannot stall the mirror.

### De-duplication

Every sync replays the whole stores, so the FCU sees the same records again:

- `M`: de-duplicate by `from` + `t` + `txt` (there is no packet ID).
- `D`: de-duplicate by `from` + `id`.

Live `P`/`U`/`R` lines for the same messages are separate records; correlate them with `D` by `from` + `id`.

## Hidden NCTNL_CTRL text

On the M5, broadcast text on the `NCTNL_CTRL` channel is now handled like `NCTNL_DATA`:

- not stored in the chat history, not shown on screen and not set as the last message;
- no notification (buzzer, LED, vibration);
- still relayed, and still passed to the phone;
- recorded in the NCTNL text store and replayed as `D` lines.

`NCTNL_DATA` keeps its assistance-alert banner and siren.

## Drops, sequence numbers and blocking

- Before writing, the node checks the free space in the console's transmit buffer. If the whole line (including the leading `\r\n`) does not fit, the line is dropped: nothing is written, the sequence number is still used, and `drop` in the next hello goes up by one. `E` lines follow the same rule. Replay lines wait for room first, see [pacing](#replay-after-a-sync).
- The console UART has a 2048-byte transmit ring buffer in mirror builds, so normal lines fit. The largest `R`/`T`/`P`/`U` line is about 560 bytes; the largest possible `H` line is about 1,370 bytes.
- Lines that are not written because of API mode or because the mirror is disabled are not attempts: they use no sequence number and do not count as drops.

## Worked examples

These lines were produced by the firmware's line formatter (`src/modules/NctnlFcuLine.h`) in a host test and decoded with the Python code [below](#decoding-in-python). The leading `\r\n` is not shown. All names, numbers and coordinates are placeholders.

```text
@NCTNL1 H 9 eyJ2IjoyLCJub2RlIjoxLCJodyI6IlRISU5LTk9ERV9NNSIsImZ3IjoiMi43LjI2LjkwMDcwY2EiLCJuYiI6IjkwMDcwY2EiLCJ1cCI6NSwiYm9vdHMiOjEyLCJjaCI6WyIiLCJPcHMiLCIiLCIiLCIiLCIiLCIiLCIiXSwiZHJvcCI6MCwiZmN1IjoxLCJsbiI6IlRlc3QgXCJOb2RlXCIgXFwg4pyTIPCfmoAiLCJzbiI6IvCfmoAiLCJncHMiOjF9 *bd125a66
```

`{"v":2,"node":1,"hw":"THINKNODE_M5","fw":"2.7.26.90070ca","nb":"90070ca","up":5,"boots":12,"ch":["","Ops","","","","","",""],"drop":0,"fcu":1,"ln":"Test \"Node\" \\ ✓ 🚀","sn":"🚀","gps":1}`, so `ln` is `Test "Node" \ ✓ 🚀`.

```text
@NCTNL1 K 10 sync eyJvayI6MX0= *38978691
```

`cmd = "sync"`, `{"ok":1}`

```text
@NCTNL1 K 11 sync eyJvayI6MCwid2h5IjoidG9vX3Nvb24iLCJyZXRyeV9zIjo0MTJ9 *21ccde21
```

`cmd = "sync"`, `{"ok":0,"why":"too_soon","retry_s":412}`

```text
@NCTNL1 G 12 eyJmaXgiOjEsImxhdCI6MTIzNDU2Nzg5LCJsb24iOi0xMjM0NTY3ODksImFsdCI6NDIsInNhdHMiOjksImhkb3AiOjg3LCJzcGQiOjEzOSwidHJrIjoyNzAwMDAwMCwidCI6MTcwMDAwMDAwMCwiYWdlIjozfQ== *3d7ea4bf
```

`{"fix":1,"lat":123456789,"lon":-123456789,"alt":42,"sats":9,"hdop":87,"spd":139,"trk":27000000,"t":1700000000,"age":3}`: 12.3456789, -12.3456789, 42 m, HDOP 0.87, 5 km/h, track 270°.

```text
@NCTNL1 G 13 eyJmaXgiOjAsInNhdHMiOjIsImFnZSI6LTF9 *c64cb19c
```

`{"fix":0,"sats":2,"age":-1}`: no fix since boot.

```text
@NCTNL1 G 14 eyJmaXgiOjAsInNhdHMiOjAsImFnZSI6NzU0fQ== *512afd6c
```

`{"fix":0,"sats":0,"age":754}`: the last fix was 754 s ago.

```text
@NCTNL1 M 15 eyJyIjoxLCJmcm9tIjoyLCJ0byI6NDI5NDk2NzI5NSwiY2giOjEsInQiOjE3MDAwMDAxMDAsImJ0IjowLCJ0eHQiOiJoZWxsbyBcIm9wc1wiXHUwMDBhbGluZSAyIPCfkY0ifQ== *96098dba
```

`{"r":1,"from":2,"to":4294967295,"ch":1,"t":1700000100,"bt":0,"txt":"hello \"ops\"\nline 2 👍"}`

```text
@NCTNL1 M 16 eyJyIjoxLCJmcm9tIjoxLCJ0byI6MiwiY2giOjAsInQiOjM3LCJidCI6MSwidHh0IjoiYm9vdC1yZWxhdGl2ZSBETSJ9 *19875e62
```

`{"r":1,"from":1,"to":2,"ch":0,"t":37,"bt":1,"txt":"boot-relative DM"}`: a DM sent by this node 37 s after a boot with no valid time.

```text
@NCTNL1 D 17 eyJyIjoxLCJmcm9tIjoyLCJ0byI6NDI5NDk2NzI5NSwiY2giOjIsImlkIjozMDU0MTk4OTYsInQiOjE3MDAwMDAyMDAsInR4dCI6IntcImtcIjpcInN0c1wiLFwidlwiOjEsXCJ0eXBlXCI6XCJvbmxpbmVcIixcImlkXCI6XCIwMDAwMDAwMi0xMjM0NTY3OFwiLFwibm9kZVwiOlwiITAwMDAwMDAyXCJ9In0= *f4a481de
```

`{"r":1,"from":2,"to":4294967295,"ch":2,"id":305419896,"t":1700000200,"txt":"{\"k\":\"sts\",\"v\":1,\"type\":\"online\",\"id\":\"00000002-12345678\",\"node\":\"!00000002\"}"}`: an NCTNL_DATA event from `!00000002`.

A command line from the FCU (no CRC, no sequence number):

```text
@NCTNL1 C sync
```

The v1 examples below are unchanged in v1.1. A v1 firmware hello, for reference:

```text
@NCTNL1 H 0 eyJ2IjoxLCJub2RlIjoxLCJodyI6IlRISU5LTk9ERV9NNSIsImZ3IjoiMi43LjI2LjkwMDcwY2EiLCJuYiI6IjkwMDcwY2EiLCJ1cCI6NSwiYm9vdHMiOjEyLCJjaCI6WyIiLCJPcHMiLCIiLCIiLCIiLCIiLCIiLCIiXSwiZHJvcCI6MCwiZmN1IjoxfQ== *8bdb8570
```

`{"v":1,"node":1,"hw":"THINKNODE_M5","fw":"2.7.26.90070ca","nb":"90070ca","up":5,"boots":12,"ch":["","Ops","","","","","",""],"drop":0,"fcu":1}`

```text
@NCTNL1 B 1 eyJtdiI6LTEsInBjdCI6LTEsInVzYiI6MSwiY2hnIjoxfQ== *2ef13fa7
```

`{"mv":-1,"pct":-1,"usb":1,"chg":1}` (before the first power reading)

```text
@NCTNL1 B 2 eyJtdiI6NDAxMiwicGN0Ijo4NywidXNiIjoxLCJjaGciOjF9 *5ecc9702
```

`{"mv":4012,"pct":87,"usb":1,"chg":1}`

```text
@NCTNL1 R 3 DQIAAAAV/////xgIKgrerb7vAQIDBAUGNXhWNBI9APFTZUUAAMhASAJgqf//////////AXgDmAEC *139aac18
```

| Field                     | Value                                    |
| ------------------------- | ---------------------------------------- |
| `from`                    | `2` (`!00000002`)                        |
| `to`                      | `4294967295` (broadcast)                 |
| `channel`                 | `8` (channel hash, as received)          |
| `id`                      | `305419896` (`0x12345678`)               |
| `encrypted`               | 10 bytes `de ad be ef 01 02 03 04 05 06` |
| `rx_time`                 | `1700000000`                             |
| `rx_snr` / `rx_rssi`      | `6.25` / `-87`                           |
| `hop_start` / `hop_limit` | `3` / `2`                                |
| `relay_node`              | `2`                                      |

```text
@NCTNL1 T 4 371 DQEAAAAV/////xgIKgwBAgMEBQYHCAkKCww1DfCtC0gDeAOYAQE= *7be245ae
```

`airtime_ms = 371`; `from: 1 to: 4294967295 channel: 8 encrypted: <12 bytes> id: 195948557 hop_limit: 3 hop_start: 3 relay_node: 1`

```text
@NCTNL1 P 5 DQIAAAAV/////yITCAMSDw0VzVsHFesypPglAPFTZTV5VjQSPQXxU2VFAACwQEgDYKb//////////wF4Aw== *3fbf5856
```

`from: 2 to: 4294967295 decoded { portnum: POSITION_APP payload: <Position> } id: 305419897 rx_time: 1700000005 rx_snr: 5.5 rx_rssi: -90 hop_limit: 3 hop_start: 3`; the payload decodes as `Position { latitude_i: 123456789 longitude_i: -123456789 time: 1700000000 }`, that is 12.3456789, -12.3456789.

```text
@NCTNL1 U 6 DQEAAAAVAgAAACIJCAESBWhlbGxvNeCsaCQ9CvFTZUgDUAE= *e3fba817
```

`from: 1 to: 2 decoded { portnum: TEXT_MESSAGE_APP payload: "hello" } id: 610839776 rx_time: 1700000010 hop_limit: 3 want_ack: true`

```text
@NCTNL1 U 7 DQEAAAAVAQAAACIECAYYATXfm1cTPRTxU2VIAw== *3483cf67
```

`from: 1 to: 1 decoded { portnum: ADMIN_APP want_response: true } id: 324508639 rx_time: 1700000020 hop_limit: 3`, an admin request from the phone with its payload removed.

```text
@NCTNL1 E 8 enc UiBNZXNoUGFja2V0IGVuY29kZSBmYWlsZWQ= *dd661f48
```

`code = "enc"`, `reason = "R MeshPacket encode failed"`

## Decoding in Python

Requires the `meshtastic` Python package for the protobuf classes.

```python
import base64
import json
import zlib

from meshtastic.protobuf import mesh_pb2

PACKET_TYPES = {"R", "T", "P", "U"}
JSON_TYPES = {"H", "B", "G", "M", "D"}


def parse(line: bytes):
    """Decode one console line. Returns None for anything that isn't a mirror line."""
    line = line.strip()
    if not line.startswith(b"@NCTNL1 "):
        return None
    body, sep, crc = line.rpartition(b" *")
    if not sep or len(crc) != 8:
        raise ValueError("malformed mirror line")
    if zlib.crc32(body + b" ") != int(crc, 16):  # CRC covers '@' .. the space before '*'
        raise ValueError("CRC mismatch")
    fields = body.decode("ascii").split(" ")
    typ, seq, extra, payload = fields[1], int(fields[2]), fields[3:-1], fields[-1]
    raw = base64.b64decode(payload, validate=True)

    out = {"type": typ, "seq": seq}
    if typ in PACKET_TYPES:
        packet = mesh_pb2.MeshPacket()
        packet.ParseFromString(raw)
        out["packet"] = packet
        if typ == "T":
            out["airtime_ms"] = int(extra[0])
    elif typ in JSON_TYPES:
        out["data"] = json.loads(raw)  # JSON text is UTF-8
    elif typ == "K":
        out["cmd"] = extra[0]
        out["data"] = json.loads(raw)
    elif typ == "E":
        out["code"] = extra[0]
        out["reason"] = raw.decode("ascii")
    else:
        out["extra"], out["raw"] = extra, raw  # unknown type from a newer firmware
    return out


def command(cmd: str) -> bytes:
    """An FCU -> node command line, e.g. command("sync")."""
    return f"@NCTNL1 C {cmd}\r\n".encode("ascii")
```

Read the port line by line (115200 baud, 8N1) and pass every line to `parse()`. Empty lines and lines that do not start with `@NCTNL1 ` are ordinary debug log output and can be ignored or logged separately. Track `seq`: a jump of more than one means lines were dropped or lost, and a drop to `0` means the node rebooted. Write `command("fcu")` every 120 s, and `command("sync")` before opening a Meshtastic API session on the same port.

## Coexistence with logs

- Each mirror line is built in full and written with a single write while holding the console's log print lock and the stream API's write lock, so a mirror line never splits a log line or a framed API packet, and vice versa.
- Mirror lines do not go through `LOG_*`, so they are not truncated or filtered by log level.
- The serial port is a hardware UART with no flow control, so the console always drains at the baud rate even with nothing attached.
