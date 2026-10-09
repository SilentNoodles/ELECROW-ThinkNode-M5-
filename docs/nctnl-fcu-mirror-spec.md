# NCTNL FCU Serial Mirror — Line Format v1

The FCU serial mirror lets an attached FCU (for example a Raspberry Pi on the node's USB serial port) log everything the node sends, receives and exchanges with its phone, without connecting as a Meshtastic API client. The phone stays connected over Bluetooth as normal.

The mirror only observes. It never touches the toPhone queue, the MeshService queues or any packet flow; it writes extra text lines to the serial console.

## Availability

| Gate            | Value                                                                                                 |
| --------------- | ----------------------------------------------------------------------------------------------------- |
| Build flag      | `NCTNL_FCU_MIRROR=1` (set for `thinknode_m5` only). Without it none of the mirror code is compiled.   |
| Runtime setting | `moduleConfig.nctnl.fcu_integration_disabled` (`NctnlConfig` field 3). `false`/unset = mirror **on**. |
| Independent of  | Debug logging, serial log levels, `debug_log_api_enabled`, `moduleConfig.nctnl.enabled`.              |

The setting is inverted on purpose: devices that already have a saved config read the new field as `false`, so the mirror is on after the update. There is no on-device toggle yet; the current state is shown as `FCU:On`/`FCU:Off` on the NCTNL Settings status page.

## Line format

```text
@NCTNL1 <type> <seq> <payload> *<crc>\r\n
```

| Field       | Meaning                                                                                                                                          |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| `@NCTNL1`   | Fixed tag. The `1` is the format version.                                                                                                        |
| `<type>`    | One ASCII letter, see [Types](#types).                                                                                                           |
| `<seq>`     | Decimal `uint32`. Starts at 1 at boot and increases by 1 for every mirror line of any type. A gap means lines were lost.                         |
| `<payload>` | Standard RFC 4648 base64 **with** `=` padding of a nanopb-encoded `meshtastic.MeshPacket`. Type `H` is plain ASCII instead.                      |
| `<crc>`     | 8 lowercase hex digits: CRC-32 (IEEE 802.3, identical to Python `zlib.crc32`) over every byte from `@` up to and including the space before `*`. |

Fields are separated by exactly one space. The base64 alphabet contains no spaces, so a line splits into exactly five parts. Lines end with `\r\n`.

The CRC is the standard reflected CRC-32: polynomial `0xEDB88320`, initial value `0xFFFFFFFF`, final XOR `0xFFFFFFFF`. Check value: `crc32("123456789") = cbf43926`.

All mirror bytes are 7-bit ASCII, so a line can never contain the Meshtastic stream framing byte `0x94`.

## Types

| Type | Source                                                                                                  | Packet form                                                                                                                |
| ---- | ------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- |
| `R`  | Every packet received over LoRa, at the point the firmware logs `Lora RX` (before duplicate filtering). | As received: usually `encrypted`, with `rx_snr`, `rx_rssi`, `rx_time`, `hop_start`, `hop_limit`, `relay_node`, `next_hop`. |
| `T`  | Every packet transmitted, at the point the firmware logs `Completed sending`. Includes relays.          | As transmitted (on-air, usually `encrypted`).                                                                              |
| `P`  | Every packet passed to `MeshService::sendToPhone()`, mirrored before it is queued.                      | Decoded where the node could decrypt it.                                                                                   |
| `U`  | Every `MeshPacket` the phone sends (`MeshService::handleToRadio()`).                                    | Decoded. `from` is set to this node's number in the mirrored copy only.                                                    |
| `H`  | Hello: once at start-up (as soon as config is loaded) and every 60 s.                                   | Plain ASCII, see below.                                                                                                    |

`P` and `U` skip packets whose decoded `portnum` is `ADMIN_APP`, because they can carry keys and configuration. Admin packets still appear in `R`/`T` in their encrypted form.

A packet can appear several times with different types, for example an incoming text message appears as `R` (encrypted, from the radio) and then `P` (decoded, to the phone). Use `id` and `from` to correlate them.

### Hello payload

```text
node=!xxxxxxxx fw=<APP_VERSION> mirror=1 up=<uptime s> mv=<battery mV> pct=<battery %> usb=<0|1> chg=<0|1>
```

| Key      | Meaning                                                                                                    |
| -------- | ---------------------------------------------------------------------------------------------------------- |
| `node`   | This node's ID.                                                                                            |
| `fw`     | Firmware `APP_VERSION`.                                                                                    |
| `mirror` | Mirror format version (1).                                                                                 |
| `up`     | Seconds since boot (`millis()/1000`, wraps after about 49.7 days).                                         |
| `mv`     | Battery voltage in mV from `powerStatus`. `-1` or `0` until the first battery reading.                     |
| `pct`    | Battery percentage from `powerStatus` (the real value, not the 101 % that telemetry reports on USB power). |
| `usb`    | 1 when USB/external power is present.                                                                      |
| `chg`    | 1 when charging.                                                                                           |

The first hello is written before the power subsystem has taken a reading, so its battery fields may be 0. Later hellos carry real values.

## Worked example

```text
@NCTNL1 R 2 DQIAAAAV/////xgIKgrerb7vAQIDBAUGNXhWNBI9APFTZUUAAMhASAJgqf//////////AXgDmAEC *5863db49
```

| Part                      | Value                                                  |
| ------------------------- | ------------------------------------------------------ |
| Type                      | `R` (received over LoRa)                               |
| Seq                       | `2`                                                    |
| CRC                       | `5863db49` = `zlib.crc32(b"@NCTNL1 R 2 DQIA...mAEC ")` |
| `from`                    | `2` (`!00000002`)                                      |
| `to`                      | `4294967295` (broadcast)                               |
| `channel`                 | `8` (channel hash, as received)                        |
| `id`                      | `305419896` (`0x12345678`)                             |
| `encrypted`               | 10 bytes `de ad be ef 01 02 03 04 05 06`               |
| `rx_time`                 | `1700000000`                                           |
| `rx_snr`                  | `6.25`                                                 |
| `rx_rssi`                 | `-87`                                                  |
| `hop_start` / `hop_limit` | `3` / `2`                                              |
| `relay_node`              | `2`                                                    |

A hello line looks like:

```text
@NCTNL1 H 1 node=!00000001 fw=2.7.26.abcdef0 mirror=1 up=5 mv=4012 pct=87 usb=1 chg=1 *d524d6d9
```

## Decoding in Python

Requires the `meshtastic` Python package for the protobuf classes.

```python
import base64
import zlib

from meshtastic.protobuf import mesh_pb2


def parse(line: bytes):
    """Return (type, seq, payload) for a mirror line, or None for any other console output."""
    line = line.rstrip(b"\r\n")
    if not line.startswith(b"@NCTNL1 "):
        return None
    body, _, crc = line.rpartition(b"*")
    if len(crc) != 8 or zlib.crc32(body) != int(crc, 16):
        raise ValueError("CRC mismatch")
    _, typ, seq, payload = body[:-1].split(b" ", 3)
    typ = typ.decode()
    if typ == "H":
        return typ, int(seq), dict(kv.split("=", 1) for kv in payload.decode().split(" "))
    packet = mesh_pb2.MeshPacket()
    packet.ParseFromString(base64.b64decode(payload, validate=True))
    return typ, int(seq), packet
```

Read the port line by line (115200 baud, 8N1) and pass every line to `parse()`. Lines that do not start with `@NCTNL1 ` are ordinary debug log output and can be ignored or logged separately.

## Coexistence with logs and API clients

- Each mirror line is built in full and written with a single write while holding the console's log print lock and the stream API's write lock, so a mirror line never splits a log line or a framed API packet, and vice versa.
- Mirror lines do not go through `LOG_*`, so they are not truncated or filtered by log level.
- If a Meshtastic serial API client connects, the mirror keeps writing. Meshtastic stream clients treat bytes outside a `0x94 0xC3` frame as debug text (the same way they already handle plain log lines), so mirror lines show up there as log output and do not break framing.
- The serial port is a hardware UART with no flow control, so writes never stall waiting for a reader; at most a write waits for the UART to drain at the baud rate.
