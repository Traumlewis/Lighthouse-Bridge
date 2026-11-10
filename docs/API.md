# Lokale HTTP-API

Basisadresse: `http://<ESP32-IP>`. Alle untenstehenden Endpunkte benötigen
`X-API-Key: <Zugangsschlüssel>` oder HTTP Basic (`admin` + Zugangsschlüssel).
Der individuelle Schlüssel steht nach jedem Start im seriellen Monitor.
Mutationen benötigen `Content-Type: application/json`. CORS wird nicht geöffnet.

## GET /api/status

Liefert Version, IP/Hostname, WLAN-/Alexa-Initialisierung, Uptime, freien Heap,
vier Stationskonfigurationen und deren Laufzeitstatus, Suchstatus und Ergebnisse.
`alexa_started` bedeutet nur, dass die SSDP-Initialisierung erfolgreich war,
nicht dass ein Echo das Gerät erkannt hat.

Pro Station unter anderem:

```json
{
  "id": 1,
  "name": "Basisstation 1",
  "mac": "AA:BB:CC:DD:EE:01",
  "address_type": 0,
  "configured": true,
  "requested": "sleep",
  "pending": false,
  "busy": false,
  "command_sent": true,
  "last_sent_ms": 35000,
  "observed": "sleep",
  "observed_raw": 0,
  "observed_ms": 35000,
  "error": ""
}
```

`requested` ist nach dem Start `null`. `observed` ist eine einmalige Messung nach
einem Befehl, kein Livezustand. Unbekannter Rohwert: `-1`. Zeitangaben sind
Millisekunden seit Start; für das Alter die Differenz zu `uptime_ms` als
vorzeichenlosen 32-Bit-Wert berechnen. Ein fehlgeschlagener neuer Auftrag kann
eine ältere Messung behalten. `command_sent` bezieht sich auf den jüngsten Auftrag;
`last_sent_ms` auf den letzten erfolgreichen Versand. Zustände werden nicht über
Neustarts gespeichert.

## POST /api/power

```json
{"station":"all","state":"sleep"}
```

Oder eine einzelne Station (numerische ID 1 bis 4):

```json
{"station":2,"state":"on"}
```

Nur **`on` und `sleep`** sind zulässig; `off` und `standby` werden abgelehnt.
Antwort `202 {"accepted":true}` bedeutet eingereiht, nicht abgeschlossen.
Anschließend Status abfragen; `pending`/`busy`/`error` beachten. Für `all` werden
nur konfigurierte Plätze berücksichtigt. Keine konfigurierte Zielstation: `409`.
Ein neuer Auftrag ersetzt wartende Aufträge für dieselbe Station; es gibt keine
unbegrenzte Warteschlange. Die laufende BLE-Aktion kann noch zu Ende gehen.

Beispiel im Terminal (Schlüssel lokal als `LH_KEY` setzen):

```sh
curl --fail-with-body http://192.168.1.42/api/power \
  -H "X-API-Key: $LH_KEY" \
  -H 'Content-Type: application/json' \
  --data '{"station":"all","state":"sleep"}'
```

## POST /api/scan

Body `{}`. Antwort `202`. Die fünfsekündige Suche läuft nach wartenden
Schaltaufträgen. Ergebnisse über `GET /api/status` lesen: `discovered` enthält
Name, MAC, `address_type` und RSSI. Es werden nur `LHB-…`-Namen angeboten, maximal
16 Ergebnisse. `scan_truncated` meldet eine Begrenzung. Kein Fund ändert die
gespeicherte Zuordnung automatisch. Eine weitere Suche während einer laufenden
Suche erzeugt keinen zusätzlichen Auftrag.

## POST /api/config

Alle vier Plätze gemeinsam speichern; eine leere MAC deaktiviert einen Platz:

```json
{
  "stations": [
    {"name":"Basisstation 1","mac":"AA:BB:CC:DD:EE:01","address_type":0},
    {"name":"Basisstation 2","mac":"AA:BB:CC:DD:EE:02","address_type":0},
    {"name":"Basisstation 3","mac":"AA:BB:CC:DD:EE:03","address_type":0},
    {"name":"Basisstation 4","mac":"AA:BB:CC:DD:EE:04","address_type":0}
  ]
}
```

`address_type`: `0` öffentlich, `1` random; aus dem Suchergebnis übernehmen.
Namen: eindeutig, 1–40 UTF-8-Bytes, keine Steuerzeichen und keine Zeichen
`"`, `\`, `<`, `>`, `&`. „VR Basisstationen“ ist reserviert. MAC-Adressen dürfen
nicht doppelt vorkommen und werden in Großbuchstaben normalisiert.
Antwort `200 {"saved":true}`. Während Suche oder Schaltaufträgen `409`; keine
Teiländerung. Fehler beim Speichern in NVS: `500`. Eine geänderte Adresse löscht
den alten Laufzeitstatus dieses Platzes.

## POST /api/wifi

Body `{}`. Startet das Setup-Portal, Antwort:
`202 {"accepted":true,"setup_ssid":"Lighthouse-Setup-XXXXXX"}`.
Danach ist die normale API bis zum Neustart nicht verfügbar. Bei laufenden
BLE-Aufträgen `409`. Gespeicherte Stationen bleiben erhalten.

## Fehler

- `400`: ungültiges JSON, ungültiger Zustand oder ungültige Konfiguration.
- `401`: Anmeldung/Schlüssel fehlt oder ist falsch.
- `403`: unerlaubter Browser-Origin.
- `409`: keine passende Station oder momentan belegt.
- `413`: JSON-Anfrage über 2400 Zeichen.
- `415`: falscher Content-Type für eine Mutation.
- `500`: Konfiguration konnte nicht dauerhaft gespeichert werden.

Anfragekörper müssen zusätzlich in das JSON-Dokument des jeweiligen Endpunkts
passen; übergroße/zu komplexe JSON-Objekte werden mit `400` abgelehnt.
JSON-Fehlerantworten haben ein `error`-Feld; die Basic-Auth-Challenge bei `401`
ist keine JSON-Antwort.

Die Alexa-/Hue-Endpunkte `/description.xml`, `/api` und
`/api/<username>/lights/...` gehören nicht zur geschützten Verwaltungs-API.
Sie bleiben für den Echo ohne Schlüssel nutzbar.
