# Node Status

**Status: Implemented and physically verified**

NCTNL reuses Meshtastic's existing Status Message functionality instead of creating a separate NCTNL status database.

## Format

The expected format is:

`{Rough Location} | {Status} | {Role} | NCTNL.io`

Example:

`Example Area | Online | Field Node | NCTNL.io`

## Ownership of each field

**Rough Location** is manually configured.

**Status** may be modified automatically by NCTNL.

**Role** is manually configured.

**NCTNL.io** identifies the expected NCTNL status format.

## Automatic mappings

| Quick Message | Status |
| --- | --- |
| Going Offline | Offline |
| Going Standby | Standby |
| Check In | Online |
| Need Assistance | Needs Assistance |
| All Clear | Online |

## Automatic changes outside Quick Messages

| Trigger | Status |
| --- | --- |
| Every start-up (after about 45 s) | Online |
| Battery Emergency | Offline |
| Charging all-clear after a battery-set Offline | The status before the battery Emergency |

A manual status action after a battery Emergency cancels the charging restore.

## NCTNL Quiet

NCTNL Quiet is on while the status is Offline or Standby and off when it is Online. While on, broadcast messages do not trigger the buzzer, LED or vibration; direct messages still alert and NCTNL alert tones are unaffected. Quiet is held in RAM only and is separate from Meshtastic's Mute, which is unchanged.

## Fail-safe parsing

Automatic modification only takes place when the existing status matches the expected four-part NCTNL format.

The firmware validates the four components, preserves Rough Location and Role, and replaces only the Status component.

If the existing text is not recognised as valid NCTNL status format, NCTNL skips the automatic change rather than rewriting arbitrary Status Message content.

## Live refresh

An early implementation persisted and broadcast the changed status successfully, but the locally connected client could continue showing stale data until the User Settings status page was opened.

The final implementation ensures locally generated status advertisements are also sent to the connected client so the visible status updates live.
