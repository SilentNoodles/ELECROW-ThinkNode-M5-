# NCTNL FCU Serial Mirror — Line Format v1

The FCU serial mirror lets an attached FCU (for example a Raspberry Pi on the node's USB serial port) log everything the node sends, receives and exchanges with its phone, plus node and battery status, without connecting as a Meshtastic API client. The phone stays connected over Bluetooth as normal.

The mirror only observes. It never touches the toPhone queue, the MeshService queues or any packet flow; it writes extra text lines to the serial console.

## Availability

| Gate            | Value                                                                                                               |
| --------------- | ------------------------------------------------------------------------------------------------------------------- |
| Build flag      | `NCTNL_FCU_MIRROR=1`, set for `thinknode_m5` and `thinknode_m2`. Without it none of the mirror code is compiled.    |
| Runtime setting | `moduleConfig.nctnl.fcu_integration_disabled` (`NctnlConfig` field 3). `false`/unset = mirror **on**.               |
| API mode        | No mirror lines while a Meshtastic serial API client is using the console (see [API clients](#serial-api-clients)). |
| Independent of  | Debug logging, serial log levels, `debug_log_api_enabled`, `moduleConfig.nctnl.enabled`.                            |

The setting is inverted on purpose: devices that already have a saved config read the new field as `false`, so the mirror is on after the update. There is no on-device toggle yet; on the M5 the current state is shown as `FCU:On`/`FCU:Off` on the NCTNL Settings status page (the M2 has no such page).

## Line format

```text
\r\n@NCTNL1 <type> <seq> [<extra> ...] <payload> *<crc>\r\n
```

| Field       | Meaning                                                                                                                                                  |
| ----------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `\r\n`      | Every line is preceded by a line break so it always starts on a fresh line, even if a log line was cut short. It is not part of the CRC.                 |
| `@NCTNL1`   | Fixed tag. The `1` is the format version.                                                                                                                |
| `<type>`    | One ASCII letter, see [Types](#types).                                                                                                                   |
| `<seq>`     | Decimal `uint32`. Starts at `0` at boot and increases by 1 for every line the node tries to write, including dropped lines. A gap means lines were lost. |
| `<extra>`   | Zero or more type-specific fields (decimal or short tokens). Only `T` and `E` have one in v1.                                                            |
| `<payload>` | RFC 4648 base64 **with** `=` padding, for every type.                                                                                                    |
| `<crc>`     | 8 lowercase hex digits: CRC-32 (IEEE 802.3, identical to Python `zlib.crc32`) over every byte from `@` up to and including the space before `*`.         |

Fields are separated by exactly one space and lines end with `\r\n`. The base64 alphabet contains no spaces, so the payload is always the last field before ` *`. Every mirror byte is 7-bit ASCII, so a line can never contain the Meshtastic stream framing byte `0x94`.

The CRC is the standard reflected CRC-32: polynomial `0xEDB88320`, initial value `0xFFFFFFFF`, final XOR `0xFFFFFFFF`. Check value: `crc32("123456789") = cbf43926`.

## Types

| Type | Extra fields   | When                                                                                                    | Payload (after base64 decoding)                                                              |
| ---- | -------------- | ------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------- |
| `H`  | —              | Hello: at boot as soon as the node number is known, then every 60 s.                                    | Compact JSON, see [Hello](#h-hello).                                                         |
| `B`  | —              | Battery: every 60 s, and within about 1 s of a change in USB power or charging state.                   | Compact JSON, see [Battery](#b-battery).                                                     |
| `R`  | —              | Every packet received over LoRa, at the point the firmware logs `Lora RX` (before duplicate filtering). | `meshtastic.MeshPacket` as received: usually `encrypted`, with `rx_*`, hop and relay fields. |
| `T`  | `<airtime_ms>` | Every packet transmitted, at the point the firmware logs `Completed sending`. Includes relays.          | `meshtastic.MeshPacket` as transmitted (on-air, usually `encrypted`).                        |
| `P`  | —              | Every packet passed to `MeshService::sendToPhone()`, before it is queued for the phone.                 | `meshtastic.MeshPacket`, decoded where the node could decrypt it.                            |
| `U`  | —              | Every `MeshPacket` the phone sends (`MeshService::handleToRadio()`).                                    | `meshtastic.MeshPacket`, decoded. `from` is set to this node's number in the mirrored copy.  |
| `E`  | `<code>`       | A line could not be built.                                                                              | Short ASCII reason, starting with the type letter of the line that failed.                   |

`T`'s `<airtime_ms>` is the time on air the firmware computed for the packet, the same value as the `Packet TX: <n>ms` debug log line.

`P` fires for every packet handed to the phone path, whether or not a phone is connected and whether or not the to-phone queue has room; the mirror is written before the queue is touched.

`E` codes in v1: `enc` (the `MeshPacket` could not be protobuf-encoded) and `len` (the line would not fit in the line buffer). An `E` line replaces the line that failed and uses its sequence number.

**ADMIN_APP:** in `P` and `U`, packets whose decoded `portnum` is `ADMIN_APP` are mirrored with `decoded.payload` emptied, because admin payloads can carry keys and configuration. All header fields (`from`, `to`, `id`, `channel`, `want_response`, and so on) are kept. `R`/`T` copies are mirrored as they are, since on air they are encrypted.

A packet can appear several times with different types, for example an incoming text message appears as `R` (encrypted, from the radio) and then `P` (decoded, to the phone). Use `id` and `from` to correlate them.

### H, hello

```text
{"v":1,"node":1,"hw":"THINKNODE_M5","fw":"2.7.26.90070ca","nb":"90070ca","up":5,"boots":12,"ch":["","Ops","","","","","",""],"drop":0,"fcu":1}
```

| Key     | Meaning                                                                                                               |
| ------- | --------------------------------------------------------------------------------------------------------------------- |
| `v`     | Hello format version (1).                                                                                             |
| `node`  | This node's number, decimal (`1` = `!00000001`).                                                                      |
| `hw`    | Hardware model name, e.g. `THINKNODE_M5` or `THINKNODE_M2` (the `meshtastic.HardwareModel` enum name).                |
| `fw`    | Firmware `APP_VERSION`.                                                                                               |
| `nb`    | Build label: the git short hash from `APP_VERSION`, or the PlatformIO env name if the version has no hash.            |
| `up`    | Seconds since boot (does not wrap at 49.7 days).                                                                      |
| `boots` | Boot counter (`myNodeInfo.reboot_count`).                                                                             |
| `ch`    | Channel names by slot index, 8 entries. `""` for a channel that uses the default (preset) name or for an unused slot. |
| `drop`  | Number of mirror lines dropped since boot because the console was busy.                                               |
| `fcu`   | `1` when the FCU mirror is enabled. Hello lines are only written while it is enabled, so this is always `1` in v1.    |

The hello never contains keys (no PSKs, no public or private keys). Channel names are UTF-8.

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

## Drops, sequence numbers and blocking

- Before writing, the node checks the free space in the console's transmit buffer. If the whole line (including the leading `\r\n`) does not fit, the line is dropped: nothing is written, the sequence number is still used, and `drop` in the next hello goes up by one. `E` lines follow the same rule.
- The console UART has a 2048-byte transmit ring buffer in mirror builds, so normal lines fit. The largest `R`/`T`/`P`/`U` line is about 560 bytes.
- Lines that are not written because of [API mode](#serial-api-clients) or because the mirror is disabled are not attempts: they use no sequence number and do not count as drops.

## Serial API clients

While a Meshtastic serial API client (for example the Python CLI or a phone app over USB) is using the console, the console is in protobuf mode and the mirror writes nothing. The firmware enters this mode on the first valid API frame and, in this firmware version, does not leave it until the node reboots. After an API session on the USB port, expect no mirror lines until the next reboot; the sequence number simply continues from where it stopped.

## Worked examples

These lines were produced by the firmware's line formatter (`src/modules/NctnlFcuLine.h`) in a host test and decoded with the Python code [below](#decoding-in-python). The leading `\r\n` is not shown.

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
    elif typ in ("H", "B"):
        out["data"] = json.loads(raw)
    elif typ == "E":
        out["code"] = extra[0]
        out["reason"] = raw.decode("ascii")
    else:
        out["extra"], out["raw"] = extra, raw  # unknown type from a newer firmware
    return out
```

Read the port line by line (115200 baud, 8N1) and pass every line to `parse()`. Empty lines and lines that do not start with `@NCTNL1 ` are ordinary debug log output and can be ignored or logged separately. Track `seq`: a jump of more than one means lines were dropped or lost, and a drop to `0` means the node rebooted.

## Coexistence with logs

- Each mirror line is built in full and written with a single write while holding the console's log print lock and the stream API's write lock, so a mirror line never splits a log line or a framed API packet, and vice versa.
- Mirror lines do not go through `LOG_*`, so they are not truncated or filtered by log level.
- The serial port is a hardware UART with no flow control, so the console always drains at the baud rate even with nothing attached.
