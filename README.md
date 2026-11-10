# Lighthouse Bridge — ESP32

**4 × Valve Lighthouse V2**, ein ESP-WROOM-32 / ESP-32S mit CH340 USB-C und
**Echo Dot (5. Generation)**. **Ein = ON (`0x01`), Aus = SLEEP (`0x00`), niemals
STANDBY.** Keine zusätzliche Handy-Companion-App erforderlich.

**[WebFlasher](https://traumlewis.github.io/Lighthouse-Bridge/)** ·
[Deutsch](https://traumlewis.github.io/Lighthouse-Bridge/?lang=de) ·
[Builds](https://github.com/Traumlewis/Lighthouse-Bridge/actions) ·
[Prüfbericht](docs/VALIDATION.md)

## Neu in 0.2.0

- Der WebFlasher schreibt getrennte Flashbereiche und erhält NVS mit WLAN,
  Zuordnung und Passwörtern, wenn **Erase device abgewählt** ist.
- BLE-Zustände werden beim Start, nach Befehlen und regelmäßig gelesen; zusätzlich
  gibt es „Aktuelle Zustände abfragen“. Angeforderte und gemessene Werte bleiben
  getrennt. Alexa erhält dieselben Messwerte bei ihren Hue-Abfragen.
- Das lokale Webinterface ist standardmäßig Englisch, mit Deutsch als zweiter
  Sprache und gespeicherter Auswahl, wie die öffentliche Flasher-Seite.
- Erster Login mit API-Key; anschließend eigenes Web-Passwort festlegen. Der
  API-Key bleibt separat und im Reiter „WLAN & lokale API“ sichtbar.
- Der WebFlasher kann Schlüssel und WLAN-Adresse über USB anzeigen. Dafür nach
  dem Flashen den Installationsdialog schließen oder „Read key & address“ wählen.

**Wichtige Update-Korrektur:** Das bisherige zusammengeführte Image enthält
`0xff`-Füllbytes über dem NVS-Bereich. Beim Schreiben an `0x0` gehen Einstellungen
auch ohne vollständiges Löschen verloren. Deshalb verwendet der WebFlasher ab
0.2.0 vier getrennte Teile. Das weiterhin verfügbare **Factory-BIN an `0x0` ist nur
für Neuinstallationen**. Die Partitionstabelle wurde nicht verändert.

Der Besitzer hat den erfolgreichen Start von 0.1.1 bestätigt. Die Erweiterungen
von 0.2.0 sind kompiliert und mit simuliertem Backend/USB geprüft; ihr kompletter
Test an Basisstationen und Echo steht noch aus. Details im Prüfbericht.

## Hardware und erster Start

Klassischer Dual-Core-ESP32, mindestens **4 MB Flash**, 2,4-GHz-WLAN. Nicht für
ESP32-S2/S3/C3. Ein USB-Datenkabel genügt, keine zusätzliche Verdrahtung. Die vier
Stationen bleiben normal mit Strom versorgt; ESP32 in BLE-Reichweite aufstellen.

1. WebFlasher in Chrome oder Edge **am Rechner** öffnen, Board anschließen,
   bestätigen und „Connect & install“ wählen.
2. Bei einem Update **Erase device abwählen**. Löschen nur bewusst für einen
   vollständigen Neuanfang; dabei gehen alle Einstellungen verloren.
3. Nach dem Flashen den Installationsdialog schließen. Der USB-Infobereich
   übernimmt den bereits freigegebenen Port und zeigt den individuellen Key und
   das Setup-WLAN. Alternativ dort „Read key & address“ drücken. Keine anderen
   seriellen Programme dürfen den Port gleichzeitig geöffnet haben.
4. Bei der ersten Einrichtung mit `Lighthouse-Setup-XXXXXX` verbinden; das
   Passwort ist der Key. `http://192.168.4.1` öffnen und Heim-WLAN speichern.
   USB kann verbunden bleiben; der Flasher zeigt die Adresse, sobald WLAN steht.
5. Im Heim-WLAN die angezeigte Webadresse öffnen. **Benutzer `admin`**, beim
   ersten Login **API-Key als Passwort**. Anschließend ein eigenes Web-Passwort
   festlegen (8–128 Bytes, verschieden vom API-Key).
6. Im Webinterface die vier `LHB-…`-Geräte suchen, jedem Platz genau eines zuordnen
   und speichern. Der BLE-Adresstyp wird aus der Suche übernommen.
7. Die Zustände werden automatisch gelesen. Zunächst eine Station einzeln mit
   ON/SLEEP prüfen, danach alle gemeinsam. Keine automatischen Schaltbefehle beim
   Start, Neustart oder Wiederverbinden.

Fehlt ein USB-Port, CH340-Treiber und Datenkabel prüfen. Bei „Connecting…“ BOOT
halten, kurz EN/RESET drücken, BOOT nach Verbindungsaufbau loslassen. Für einen
normalen Neustart nur EN/RESET drücken. Der Key steht außerdem im seriellen
Monitor bei 115200 Baud. Der WebFlasher kann Geräteinfos erst ab Firmware 0.2.0
abfragen; bei älteren Versionen zunächst aktualisieren.

## Gemessener Status und Alexa

Der ESP32 liest die Power-Characteristic der konfigurierten Stationen:

| Rohwert | Anzeige | Hue `on` |
| --- | --- | --- |
| `0x00` | SLEEP | false |
| `0x01`, `0x08`, `0x09` | Starting / Startet | true |
| `0x0b` | ON | true |
| `0x02` | STANDBY (nur gelesen) | false |
| Keine erfolgreiche Messung | Unknown / Unbekannt | zuletzt bekannter Wert, `reachable: false` |

Es gibt einen BLE-Worker und immer nur eine Verbindung gleichzeitig. Befehle
haben Vorrang, mit höchstens drei Versuchen. Messungen folgen beim Start, nach
Befehlen und im Zielintervall von 15 Sekunden; startende Stationen werden nach
etwa zwei Sekunden, nicht erreichbare nach etwa 30 Sekunden erneut gelesen.
Verbindungsversuche und andere Stationen können diese Intervalle verlängern.
Eine manuelle Aktualisierung reiht reine Leseaufträge ein, ohne Schaltbefehl.

`requested` ist ein Auftrag, **kein Messwert**. Webinterface und Hue-Antworten
verwenden `observed`. Bei Lesefehlern bleibt der alte Wert mit Zeitstempel erhalten,
gilt aber als nicht erreichbar. Messungen ab 60 Sekunden gelten ebenfalls als
veraltet. Das Webinterface fragt den ESP32-Speicher alle 2,5 Sekunden ab.

Echo Dot und ESP32 müssen im selben Heimnetz sein, ohne Client-Isolation.
„Alexa, suche Geräte“ findet vier einzelne Lampen und **„VR Basisstationen“**.
Gespeicherte Namen bleiben erhalten; neue Plätze heißen „Base station 1“ usw.
Der virtuelle Gesamtschalter gilt als an, wenn alle zugeordneten Stationen aktiv
sind, und als erreichbar, wenn alle frische Messwerte haben.

**Alexa erhält bei GET-Abfragen der lokalen Hue-Bridge dieselben Messwerte.
Dieses Protokoll bietet keinen Push in die Alexa-App.** Die App kann ihren
Schalter weiterhin zwischenspeichern, bis Alexa erneut abfragt. Sofortige
Aktualisierung der Alexa-Anzeige lässt sich damit nicht garantieren. Alexa
bestätigt Befehle außerdem vor Abschluss von BLE; Fehler stehen im Webinterface.
Die Hue-Emulation muss mit der jeweiligen Echo-Firmware geprüft werden.

## WLAN, Web-Passwort und API-Key

- WLAN wechseln: „Wi-Fi & local API / WLAN & lokale API → Open Wi-Fi setup“ oder
  nach dem Start BOOT fünf Sekunden halten. Während aktiver BLE-Arbeit kurz
  warten. Das manuelle Portal pausiert die Steuerung bis zum Speichern oder
  spätestens drei Minuten und startet anschließend neu.
- Ohne erreichbares gespeichertes WLAN öffnet sich beim Start das Setup-Portal
  ohne Zeitlimit. USB-Geräteinfos bleiben dabei abfragbar. WLAN wird im normalen
  ESP32-NVS gespeichert; Updates über die neuen Flashteile erhalten diesen Bereich.
- Die Web-Anmeldung nutzt eine HttpOnly-/SameSite-Session. Das eigene Passwort
  wird nur als PBKDF2-SHA256-Hash mit individuellem Salt und 10000 Iterationen
  gespeichert, niemals angezeigt oder über USB ausgegeben. Ein Passwortwechsel
  meldet andere Sessions ab. Sessions verfallen nach spätestens zwölf Stunden im Browser
  oder einem Neustart.
- Der **API-Key** bleibt für `X-API-Key`-Integrationen und als Passwort des
  Setup-WLANs erhalten. Nach Vergabe eines Web-Passworts funktioniert er nicht mehr
  als Web-Login. Angemeldete Benutzer sehen ihn unter „WLAN & lokale API“.
- Passwort vergessen: über USB bei 115200 Baud `LH_RESET_WEB_PASSWORD` mit
  Zeilenumbruch senden. Das setzt nur das Web-Passwort und Sessions zurück; dann
  erneut mit dem API-Key anmelden und ein neues Passwort wählen. WLAN, Key und
  Stationszuordnung bleiben erhalten.

HTTP und NVS sind unverschlüsselt. Die Hue-Endpunkte sind für Alexa bewusst ohne
Anmeldung nutzbar; deshalb nur im vertrauenswürdigen Heimnetz betreiben, ohne
Internet-Portweiterleitung. Schlüssel/Adressen aus USB bleiben im Speicher der
Flasher-Seite und werden nicht an GitHub übertragen oder im Browser gespeichert.

## Lokale API und spätere Pebble-Anbindung

[API-Dokumentation](docs/API.md). Ein späteres Pebble-Plugin kann mit LocalNetwork
und `X-API-Key` dieselben Endpunkte nutzen. Ein solches Plugin ist noch nicht
implementiert; auch OTA und Improv Serial sind nicht Bestandteil dieser Firmware.

## Selbst bauen, prüfen und manuell flashen

In PlatformIO dieses Projekt öffnen, Profil `esp32dev` verwenden:

```sh
pio run
pio run -t upload --upload-port COM5
pio device monitor --port COM5 --baud 115200
```

`COM5` durch deinen Port ersetzen. Für ein manuelles **einstellungserhaltendes**
Update aus dem ZIP (esptool 4.5.1):

```sh
python -m esptool --chip esp32 --port COM5 --baud 115200 write_flash \
  0x1000 lighthouse-bridge-bootloader.bin \
  0x8000 lighthouse-bridge-partitions.bin \
  0xe000 lighthouse-bridge-boot-app0.bin \
  0x10000 lighthouse-bridge-app.bin
```

`lighthouse-bridge-esp32-merged.bin` an `0x0` überschreibt dagegen NVS und ist
nur ein Factory-Image. Die zwei Anwendungsslots bleiben für spätere Erweiterungen
vorhanden; OTA ist noch nicht implementiert. Das gesamte Webinterface steckt in
der Firmware, kein Dateisystem-Upload nötig.

```sh
g++ -std=c++11 -Wall -Wextra -Werror -I include test/test_core.cpp -o /tmp/lh-test
/tmp/lh-test
python tools/package_release.py
python tools/build_pages.py
python -m unittest discover -s test -p 'test_pages.py' -v
node test/serial_info.cjs
npm install
npm test
```

Für eigene PlatformIO-Verzeichnisse `PLATFORMIO_CORE_DIR` oder `--pio-core-dir`
beim Paketieren setzen; Chromium über `CHROMIUM_PATH` auswählen. Browserprüfungen
und USB-Simulation ersetzen keinen Hardwaretest. [WebFlasher-Details](docs/WEBFLASHER.md),
[Quellen und Bibliotheken](docs/SOURCES.md), [Projektvorgaben](AGENTS.md).
