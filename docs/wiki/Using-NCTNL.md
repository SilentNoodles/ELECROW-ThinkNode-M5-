# Using NCTNL

NCTNL adds operational shortcuts to normal Meshtastic behaviour while preserving standard messaging and interoperability.

## Quick Message Menu

The NCTNL Quick Message Menu provides rapid access to predefined operational messages, Free Text and manual coordinate sharing.

The existing Meshtastic destination-selection mechanism is reused so the user can choose a channel or an individual node rather than NCTNL creating a separate destination system.

See [Quick Message Menu](Quick-Message-Menu).

## Node Status

NCTNL reuses the existing Meshtastic Status Message feature.

The expected NCTNL format is:

`{Rough Location} | {Status} | {Role} | NCTNL.io`

NCTNL only changes the Status section automatically, and only when the existing status matches the expected four-part structure.

See [Node Status](Node-Status).

## Location

Location is not automatically transmitted simply because the Send Coordinates option exists.

Send Coordinates is a deliberate manual action and requires a valid current location fix.

See [Send Coordinates](Send-Coordinates).

## Compatibility

The five predefined NCTNL Quick Messages are ordinary Meshtastic text messages. This allows ordinary compatible Meshtastic clients and nodes to receive and read them without implementing the NCTNL structured protocol.
