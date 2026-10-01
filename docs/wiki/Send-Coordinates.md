# Send Coordinates

**Status: Implemented and physically verified**

The Quick Message Menu contains:

`[Send Coordinates]`

## Behaviour

When a valid current GPS fix is available, NCTNL sends a normal Meshtastic text message:

```text
Coordinates: <latitude>, <longitude>
```

Latitude and longitude are formatted to six decimal places.

The message uses the currently selected Meshtastic channel or individual-node destination.

## No valid GPS fix

If there is no valid current GPS fix:

- nothing is transmitted;
- the device displays **No GPS fix**; and
- stale, zero or placeholder coordinates are not intentionally sent.

## Privacy model

Send Coordinates is a deliberate manual location-sharing action.

Its existence does not mean that NCTNL automatically shares the node's location through this function.
