# Version history

Release notes are listed newest first. Current firmware: **0.3.7**.
For validation evidence and remaining hardware checks, see [the validation report](docs/VALIDATION.md).

## Version 0.3.7

Station IDs are **16 fixed slots (1–16)**. A new entry gets the lowest free ID:
first station 1, second 2. Deleting or reordering an entry leaves the other IDs
unchanged, including after reboot. New entries can reuse freed slots. The cards,
setup headers/default names, API and log all use those fixed IDs. Existing 0.3.6
assignments and remaining Alexa identities are preserved. This supersedes the
automatic renumbering introduced in 0.3.6.

## Version 0.3.6

Station numbers now match the configured list everywhere: four entries have IDs
**1–4**, shown as **STATION 01–04**. Local IDs compact after removal or reordering,
up to 16. This also applies to API and worker logs, not only card labels.
Existing IDs such as 6–9 migrate automatically. Alexa keeps a separate stable
identity, so its encoded keys and unique IDs are preserved. No reassignment is
needed. API clients must refresh the station list after configuration changes.

## Version 0.3.5

Restore the quiet normal view: detailed errors, read phases, measurement ages and
last-command history appear only in Debug. Current measured state, unavailability
and queued/sending power commands remain visible. Triple-click the version to
toggle as before. Cards reserve a fixed status area and the footer reserves its
Debug indicator space, so toggling does not move the design. Long diagnostics
scroll inside the card rather than stretching it. BLE/API behavior is unchanged.

## Version 0.3.4

The debug toggle now hides only per-station BLE error/retry messages and orange
warning highlighting. Regular read phases, last-command results, measurement ages
and unavailable/stale status remain visible. Triple-click the firmware version
to toggle warnings, as before. This corrects the overly broad filtering in 0.3.3;
BLE, Alexa and API behavior are unchanged.

## Version 0.3.3

The dashboard hides detailed read/retry errors, measurement ages and last-command
history by default. Triple-click the firmware version within 1.2 seconds to turn
**Debug** on; repeat to turn it off. It resets on reload/sign-out and supports
keyboard activation. Current measured state, unavailable stations and queued or
sending power commands remain visible. API/USB diagnostics and BLE behavior are
unchanged. Install the 0.3.3 .lhota package to update the embedded interface.

## Version 0.3.2

Fix the Hue discovery identifier format for newly added stations. Versions 0.3.0
and 0.3.1 formatted IDs above 4 as unpadded decimal endpoints instead of the
previously working zero-padded hexadecimal representation. This affects the
owner's newly assigned IDs 6–9. Existing first-four identities and numeric Hue
keys are preserved; no station reassignment is needed. Install the 0.3.2 update,
then ask Alexa to discover devices again. Actual Alexa rediscovery needs an owner
retest; the regression and serialized responses are covered by host tests.

## Version 0.3.1

The API key in the ESP32 dashboard now uses the same plain clickable text styling
as the WebFlasher: no visible button background, border or padding. Click or press
Enter to copy. Install the 0.3.1 .lhota package through the dashboard's firmware
update section, or update by USB with Erase device unchecked.

## Version 0.3.0

- Dynamic station list: add/remove up to 16 entries, with stable IDs. Existing
  assigned stations migrate automatically; old empty slots disappear.
- Only individual stations are advertised to Alexa. The old **VR Basisstationen**
  virtual light is retired. Delete that cached device once in the Alexa app;
  create Alexa groups for collective control. Existing individual IDs are kept.
- One Bluetooth-address field accepts manual entry or discovered suggestions.
  All configured stations use Random (1); there is no address-type selector.
- Background reads no longer disable/re-enable the Wi-Fi setup button.
- Click the API key to copy it, in the dashboard or USB WebFlasher panel.
- Custom setup Wi-Fi name, shared with the local `name.local` hostname.
  The setup portal uses the dashboard's dark colors and rounded controls.
- Firmware upload over Wi-Fi using an official **`.lhota`** update package.

The owner has tested four physical stations with earlier firmware, reporting
improved switching and Alexa behavior. The new 16-station capacity and OTA
require hardware verification. Intermittent BLE HCI 0x3e failures remain possible;
this release preserves the bounded transport retries and read-only recovery.

