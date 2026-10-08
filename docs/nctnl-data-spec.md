# NCTNL_DATA event specification (v1)

NCTNL nodes broadcast machine-readable events as compact JSON on the channel named `NCTNL_DATA`.

## Transport

- Destination: broadcast (`NODENUM_BROADCAST`)
- Channel: the enabled channel whose name matches `NCTNL_DATA` (case-insensitive), looked up at every send.
  If no such channel exists the event is not sent; it never falls back to another channel.
- Port: `TEXT_MESSAGE_APP`, so routers using `CORE_PORTNUMS_ONLY` relay it and standard apps show it in the channel chat.
- Payload: UTF-8 JSON, compact (no spaces), at most `meshtastic_Constants_DATA_PAYLOAD_LEN` (233) bytes.
  An event that would exceed this is not sent; JSON is never truncated.
- The sender logs the exact JSON to the serial log (split into numbered parts, as log lines are capped at 160 characters).
- Receivers identify NCTNL data by the channel name `NCTNL_DATA` plus a message body starting with `{"k":`.
- Devices should mute the `NCTNL_DATA` channel so these events do not trigger message notifications.

## Fields

Keys appear in this order.

| Key    | Description                                                                            |
| ------ | -------------------------------------------------------------------------------------- |
| `k`    | Kind, always first: `"bat"` or `"sts"`                                                 |
| `v`    | Spec version, `1`                                                                      |
| `type` | Event name (see below)                                                                 |
| `id`   | `"<node hex 8>-<packet id hex 8>"`, lowercase, e.g. `"a1b2c3d4-3f2a91c7"`              |
| `node` | `"!<node hex 8>"`                                                                      |
| `name` | Owner short name (`"` and `\` escaped)                                                 |
| `time` | Current UTC as ISO 8601 `"YYYY-MM-DDTHH:MM:SSZ"`; `null` if the clock has not been set |
| `bat`  | Battery percent                                                                        |
| `lat`  | Last known latitude, 7 decimal places                                                  |
| `lon`  | Last known longitude, 7 decimal places                                                 |
| `fix`  | UTC ISO 8601 time of the last position; `null` if unknown                              |

`lat`, `lon` and `fix` are all omitted when no position is known (latitude and longitude both 0).

### Kind `bat`

Adds:

| Key   | Description                                        |
| ----- | -------------------------------------------------- |
| `mv`  | Battery voltage in mV                              |
| `chg` | `true` when on USB power or charging, else `false` |

Types: `low`, `critical`, `emergency`, `charging`

### Kind `sts`

Adds nothing.

Types: `going_offline`, `standby`, `check_in`, `assistance`, `all_clear`

## Examples

Coordinates and identifiers are placeholders.

```text
{"k":"bat","v":1,"type":"critical","id":"a1b2c3d4-3f2a91c7","node":"!a1b2c3d4","name":"FLD1","time":"2026-10-08T22:41:26Z","bat":9,"lat":12.3456789,"lon":-12.3456789,"fix":"2026-10-08T21:58:03Z","mv":3518,"chg":false}
```

```text
{"k":"sts","v":1,"type":"check_in","id":"a1b2c3d4-3f2a91c8","node":"!a1b2c3d4","name":"FLD1","time":null,"bat":64}
```
