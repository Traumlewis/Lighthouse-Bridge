# Lokale API — 0.2.1

Basisadresse `http://<ESP32-IP>`. Für Integrationen gilt unverändert
`X-API-Key: <Key>`. Der Webbrowser verwendet stattdessen eine HttpOnly-Session.
HTTP Basic wurde durch den Web-Login ersetzt. Änderungen benötigen
`Content-Type: application/json`; CORS wird nicht geöffnet.

## Web-Anmeldung (keine Integrations-Endpunkte)

- `POST /api/login`, Body `{"password":"…"}`. Beim ersten Login ohne gespeichertes
  Web-Passwort wird der API-Key akzeptiert. Danach ausschließlich das eigene
  Passwort. Antwort `{"password_required":true|false}` und Cookie `LHSESSION`.
  Nach fünf Fehlversuchen 30 Sekunden Sperre (`429`).
- Bootstrap-Sessions müssen erst ein eigenes Passwort festlegen; normale
  geschützte Anfragen liefern vorher `428 PASSWORD_REQUIRED`.
- `POST /api/password`, Body `{"password":"…"}`: 8–128 Bytes, verschieden vom
  API-Key. Nur angemeldete Web-Sessions, **nicht** `X-API-Key`. Speichert Salt und
  PBKDF2-SHA256-Hash atomar als NVS-Eintrag, verwirft alte Sessions, erstellt eine
  neue Session. Antwort `{"saved":true}`. Passwort niemals zurückgegeben.
- `GET /api/account`: nur Web-Session, auch Bootstrap. Liefert `password_required`
  und `setup_ssid`; `api_key` wird erst nach Passwortvergabe ausgegeben. API-Key-
  Anmeldung als Header reicht für diesen Endpunkt ausdrücklich nicht.
- `POST /api/logout`, Body `{}`: Web-Session löschen. Cookie HttpOnly,
  SameSite=Strict, zwölf Stunden Inaktivität; Reboot verwirft alle Sessions.

## GET /api/status

Version, IP/Hostname, WLAN-/SSDP-Initialisierung, Uptime, freier Heap, vier Plätze,
Suchstatus und Ergebnisse. `alexa_started` bestätigt nur den SSDP-Start.
Pro Station beispielsweise:

```json
{
  "id":1,"name":"Base station 1","mac":"AA:BB:CC:DD:EE:01","address_type":0,
  "configured":true,"requested":"on","pending":false,"busy":false,
  "reading":false,"reachable":true,"command_sent":true,"last_sent_ms":35000,
  "observed":"on","observed_raw":11,"observed_ms":39000,"error":""
}
```

`requested` ist nur ein Ziel und nach Neustart `null`. `observed` ist die letzte
BLE-Messung. Gelesen wird beim Start, nach Befehlen und regelmäßig (Ziel 15 s,
Boot-Zustand 2 s, nicht erreichbar 30 s; Verzögerungen bei belegtem BLE möglich).
`reading` schließt vorgemerkte Leseaufträge ein. Jede Lese-Runde hat höchstens
zwei Versuche mit sechs Sekunden Verbindungs-Timeout. Ein einzelner Fehlschlag
erhält eine bisher gültige Messung, mit `read_failures=1` und Warnung im UI;
Wiederholung nach fünf Sekunden. Zwei erfolglose Runden, kein Messwert oder Alter
ab 60 s bedeuten `reachable=false`. Zeitstempel werden nie künstlich erneuert.
`ble_error` ist der letzte numerische NimBLE-Fehler (0 falls keiner verfügbar),
`last_read_attempt_ms` das Ende der letzten Abfrage, `read_failures` die Zahl
aufeinanderfolgender fehlgeschlagener Runden. Erfolgreiche Messung setzt sie zurück.
`observed_raw=-1` bedeutet noch keine Messung. Zeitangaben sind Millisekunden seit
Start; Differenzen als unsigned 32 Bit berechnen. Status wird nicht persistiert.

## POST /api/refresh

Body `{}`, Antwort `202 {"accepted":true}`. Reiht reine BLE-Abfragen aller
konfigurierten Plätze ein, **keine** Power-Schreibbefehle. Fortschritt in Status.

## POST /api/power

`{"station":"all","state":"sleep"}` oder `{"station":2,"state":"on"}`.
Nur `on`/`sleep`; `off`/`standby` sind ungültig. `202 {"accepted":true}` bedeutet
vorgemerkt, nicht gemessen. Neue Ziele ersetzen wartende Aufträge für denselben
Platz; laufende Aktionen können noch enden. Drei Schreib-/Verbindungsversuche
maximal. Die Gruppe berücksichtigt nur zugeordnete Plätze, sonst `409 NO_STATION`.

```sh
curl --fail-with-body http://192.168.1.42/api/power \
  -H "X-API-Key: $LH_KEY" -H 'Content-Type: application/json' \
  --data '{"station":"all","state":"sleep"}'
```

## POST /api/scan

Body `{}`, Antwort `202`. Fünfsekündige Suche nach `LHB-…`, nach Schaltaufträgen.
`discovered` enthält Name, MAC, `address_type`, RSSI; maximal 16 Ergebnisse,
`scan_truncated` meldet eine Begrenzung. Kein Fund wird automatisch zugeordnet.

## POST /api/config

Alle vier Plätze gemeinsam: `{"stations":[…]}` mit je
`{"name":"Base station 1","mac":"AA:BB:CC:DD:EE:01","address_type":0}`.
Leere MAC deaktiviert einen Platz. Typ `0` öffentlich, `1` random, aus Suche
übernehmen. Namen eindeutig, 1–40 UTF-8-Bytes, keine Steuerzeichen oder
`"`, `\`, `<`, `>`, `&`; `VR Basisstationen` ist reserviert. MACs eindeutig und
normalisiert. Erfolg `200 {"saved":true}`. Bei laufenden Schaltaufträgen oder Suche
`409 BLE_BUSY`; keine Teiländerung. Eine geänderte Adresse löscht alte Messwerte. Hintergrundmessungen blockieren
das Speichern nicht; Ergebnisse der vorherigen Zuordnung werden verworfen.

## POST /api/wifi

Body `{}`, Antwort `202 {"accepted":true,"setup_ssid":"Lighthouse-Setup-XXXXXX"}`.
Öffnet den geschützten WLAN-Zugangspunkt und pausiert BLE/Verwaltungs-HTTP.
Bei aktiver BLE-Arbeit `409`. Nach Abschluss oder spätestens drei Minuten Reboot.
Key und Zuordnung bleiben unverändert; USB-Geräteinfos bleiben abfragbar.

## Alexa-/Hue-Abfragen

`/description.xml`, `/api`, `/api/<username>/lights[/<id>[/state]]` sind ohne
Anmeldung für den Echo erreichbar. GETs erhalten **dieselben BLE-Messwerte** wie
Status, `reachable` berücksichtigt Lesefehler und Messwertalter. Light-IDs bleiben
zum bisherigen Espalexa-Schema kompatibel. Gruppen-ON: alle zugeordneten Plätze
aktiv; Gruppen-reachable: alle haben frische Werte. Anlaufwerte 1/8/9 gelten als
aktiv, aber das Dashboard zeigt weiterhin Starting. Es gibt keinen Status-Push
in die Alexa-App: Alexa entscheidet, wann sie erneut abfragt.

## USB-Informationsprotokoll

Über den physischen seriellen Port bei 115200 Baud: `LH_INFO\n` senden.
Antwort als eine Zeile `LH_INFO {…}`:

```json
{"protocol":1,"version":"0.2.0","api_key":"<24 hex characters>","setup_ssid":"Lighthouse-Setup-XXXXXX","wifi_connected":false,"url":null,"password_set":false}
```

Sobald WLAN steht, sind `wifi_connected=true` und `url=http://<IPv4>/`.
Das eigene Web-Passwort wird nie ausgegeben. Diese Informationen sind bewusst
USB-lokal; es gibt keinen ungeschützten HTTP-Endpunkt dafür. Die Flasher-Seite
fragt alle zwei Sekunden, solange ihr USB-Informationsabruf verbunden bleibt.
`LH_RESET_WEB_PASSWORD\n` setzt nur Web-Passwort und Sessions zurück, ohne WLAN,
Key oder Stationszuordnung zu löschen. Dieser Wiederherstellungsbefehl erfordert
physischen USB-Zugriff und wird nicht automatisch vom Flasher gesendet.

## Fehler

JSON-Fehler haben `error` als stabilen Code; Weboberfläche übersetzt ihn EN/DE.
`400`: ungültige Daten; `401`: Login/Key falsch oder fehlt; `403`: Origin;
`409`: belegt/kein Ziel; `413`: >2400 Zeichen; `415`: Content-Type;
`428`: eigenes Passwort fehlt; `429`: Login-Sperre; `500`: Speicher-/Hashfehler.
Anfrage muss zusätzlich ins JSON-Dokument des Endpunkts passen.
