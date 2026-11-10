# Lighthouse Bridge — ESP32

Firmware-Prototyp für **4 × Valve Lighthouse V2** und **Echo Dot (5. Generation)**.

**[WebFlasher öffnen](https://traumlewis.github.io/Lighthouse-Bridge/)** ·
[Build-Status](https://github.com/Traumlewis/Lighthouse-Bridge/actions/workflows/firmware-pages.yml)

In Chrome oder Edge auf einem Rechner kannst du den ESP32 direkt per USB flashen.
Die Flasher-Seite ist standardmäßig Englisch; Deutsch lässt sich im Sprachschalter
auswählen oder direkt [mit diesem Link](https://traumlewis.github.io/Lighthouse-Bridge/?lang=de) öffnen.
Die Webseite bietet auch die serielle Konsole für deinen Zugangsschlüssel.
Das WLAN richtest du anschließend über den Setup-Zugangspunkt des ESP32 ein.
Die vollständige Anleitung zum Veröffentlichen steht unter
[GitHub Pages](docs/WEBFLASHER.md).

- Alexa: vier einzelne schaltbare Lampen plus „VR Basisstationen“.
- **Ein = ON (`0x01`), Aus = SLEEP (`0x00`). Kein STANDBY-Befehl.**
- Geschütztes WLAN-Setup über einen eigenen Zugangspunkt.
- Lokales Webinterface mit BLE-Suche, Zuordnung, Schaltern und Fehleranzeige.
- Lokale HTTP-API für eine spätere Pebble-Anbindung.
- Kein automatisches Einschalten oder Schlafenlegen beim Start/Neustart.

**Status:** Für `esp32dev` kompiliert; siehe [Prüfbericht](docs/VALIDATION.md).
Die BLE-Kommunikation und die Alexa-Erkennung sind ohne echte Hardware noch
nicht getestet. Insbesondere die Hue-Emulation ist keine garantierte
Alexa-Kompatibilität. Das Projekt benötigt keine Handy-Companion-App.

## Hardware

Vorgesehen ist das beschriebene **ESP-WROOM-32 / ESP-32S Dual-Core Board mit
CH340 und USB-C**. Das Buildprofil nimmt den klassischen ESP32 mit **4 MB Flash**
an. Vor dem Flashen Modulaufdruck und Flashgröße prüfen: Die allgemeine
Verkaufsbezeichnung allein legt die Hardware nicht eindeutig fest. Dieses Image
ist nicht für ESP32-S2, S3 oder C3 bestimmt.

Keine zusätzlichen Bauteile oder Leitungen nötig. Den ESP32 per USB mit Strom
versorgen, im Bereich aller vier Stationen aufstellen. Er braucht 2,4-GHz-WLAN;
die Basisstationen behalten ihre normale Stromversorgung. Der CH340 ist nur der
USB-Seriell-Wandler und beeinflusst die BLE-/WLAN-Funktionen nicht.

## Schnellstart mit fertigem Image

Im bereitgestellten ZIP liegen `lighthouse-bridge-esp32-merged.bin` und diese
Anleitung. Das **zusammengeführte Image wird an Adresse `0x0`** geschrieben.
Es enthält Bootloader, Partitionstabelle und Anwendung.

1. Ein USB-Datenkabel verwenden. Falls der serielle Port fehlt, den passenden
   CH340-Treiber für dein Betriebssystem installieren.
2. Mit einem ESP32-kompatiblen Flashtool das zusammengeführte Image an `0x0`
   flashen. Alternativ mit Python/esptool 4.5.1:

   ```sh
   python -m pip install esptool==4.5.1
   python -m esptool --chip esp32 --port COM5 flash_id
   python -m esptool --chip esp32 --port COM5 --baud 115200 write_flash 0x0 lighthouse-bridge-esp32-merged.bin
   ```

   `COM5` durch deinen Port ersetzen, z. B. `/dev/ttyUSB0` unter Linux oder
   `/dev/cu.wchusbserial…` unter macOS. `flash_id` sollte 4 MB oder mehr melden;
   das Image verwendet die ersten 4 MB. War vorher eine andere Firmware auf dem
   Board, ist ein vorheriges `erase_flash` sinnvoll. Dabei gehen alle alten
   Einstellungen verloren; bei späteren Updates nicht routinemäßig löschen.
3. Einen seriellen Monitor mit **115200 Baud** öffnen und **EN/RESET** drücken.
   Das Gerät gibt seinen individuellen 24-stelligen Zugangsschlüssel aus.
   Dieser bleibt bei normalen Neustarts erhalten.
4. Mit `Lighthouse-Setup-XXXXXX` verbinden. **WLAN-Passwort = Zugangsschlüssel.**
   Falls kein Portal aufgeht: **http://192.168.4.1** öffnen.
5. Heim-WLAN auswählen und dessen Passwort speichern. Der ESP32 verbindet sich.
   Das Handy/der Rechner muss anschließend zurück ins Heim-WLAN.
6. Die im seriellen Monitor angezeigte IP im Browser öffnen. Anmeldung:
   **Benutzer `admin`, Passwort = Zugangsschlüssel.** Alternativ funktioniert
   häufig `http://lighthouse-xxxxxx.local/`; der exakte Name steht im Monitor.
7. „Basisstationen einrichten“ öffnen, Bluetooth-Suche starten, die vier
   gefundenen `LHB-…`-Stationen auf vier Plätze verteilen und speichern.
   Der richtige BLE-Adresstyp wird bei der Auswahl automatisch übernommen.
8. Erst eine Station mit ON und SLEEP testen, anschließend die Gruppe.

Wenn das Flashen bei „Connecting…“ hängt: BOOT gedrückt halten, kurz EN/RESET
drücken, dann BOOT loslassen, sobald die Verbindung steht. Für das spätere
WLAN-Setup BOOT erst **nach dem normalen Start** drücken; beim Einschalten
gehalten startet BOOT den Bootloader.

## Alexa

1. Echo Dot und ESP32 im selben lokalen Netz betreiben. 2,4 und 5 GHz dürfen
   unterschiedliche Funkbänder desselben Heimnetzes sein; Gastnetz und
   Client-Isolation verhindern häufig die Erkennung.
2. Nach der Stationszuordnung sagen: **„Alexa, suche Geräte.“** Alternativ in
   der Alexa-App eine Gerätesuche starten; falls gefragt, Philips Hue ohne
   zusätzliche Skill-Verknüpfung versuchen.
3. Erwartet werden „Basisstation 1“ bis „Basisstation 4“ (oder deine Namen)
   sowie **„VR Basisstationen“**.
4. **„Alexa, schalte VR Basisstationen ein.“** → alle zugeordneten Stationen ON.
5. **„Alexa, schalte VR Basisstationen aus.“** → alle zugeordneten Stationen SLEEP.

Die Firmware nutzt [Espalexa](https://github.com/Aircoookie/Espalexa), eine
lokale Hue-Bridge-Emulation über SSDP und HTTP. Kein Amazon-Entwicklerkonto,
eigener Skill oder Home Assistant nötig. Alexa selbst benötigt weiterhin ihre
übliche Verbindung zu Amazon. Ob die aktuelle Software deines Echo Dot 5 die
Emulation akzeptiert, muss vor Ort getestet werden. Bei erfolgloser Suche Echo
neu starten und Multicast/SSDP (UDP 1900) sowie Port 80 im Heimnetz prüfen.
Funktioniert die Erkennung damit nicht, bleibt die lokale Web-/API-Steuerung
nutzbar; eine andere Alexa-Anbindung wäre ein weiterer Entwicklungsschritt.

Alexa bestätigt Befehle, bevor BLE abgeschlossen ist. **Der Alexa-Schalter
zeigt den gewünschten Zustand, keine bestätigte Messung.** Einzelne unerreichbare
Stationen werden in der Weboberfläche als Fehler angezeigt. Alle vier
Einzellampen sind bereits vor der Zuordnung sichtbar, schalten aber erst nach
Zuordnung einer MAC-Adresse ein echtes Gerät. Nach Namensänderungen kann Alexa
alte Namen zwischenspeichern: betroffene Geräte entfernen und neu suchen.

## WLAN ändern

Im Webinterface „WLAN & lokale API → WLAN-Setup öffnen“, oder nach dem Start
**BOOT (GPIO 0) fünf Sekunden halten**, wenn kein BLE-Auftrag mehr läuft.
Dann mit dem Setup-WLAN verbinden. Der Zugangsschlüssel bleibt gleich.
Das manuell gestartete Portal läuft bis zu drei Minuten und startet den ESP32
danach neu; beim Speichern verbindet er sich mit dem neuen WLAN. Stationszuordnung
und Zugangsschlüssel bleiben erhalten. Während des Portals pausiert die normale
Bedienung. Ohne gespeichertes/erreichbares WLAN öffnet der ESP32 beim Start das
Portal ohne Zeitlimit. Während einer laufenden Sitzung versucht er bei
WLAN-Verlust zunächst automatisch, sich wieder zu verbinden.

## Bedienung und Zustandsanzeige

BLE-Zugriffe laufen nacheinander in einem separaten FreeRTOS-Task. Verbindungen
werden nach jeder Aktion geschlossen. Bei Fehlern gibt es höchstens drei
Versuche; ein neuerer Auftrag ersetzt einen noch wartenden älteren Auftrag für
dieselbe Station. Die Gruppe kann daher je nach Empfang einige Sekunden bis
über eine Minute brauchen. Ein bereits laufender Auftrag kann noch abgeschlossen
werden, bevor der neuere Auftrag folgt.

„Befehl gesendet“ bedeutet, dass der BLE-Schreibvorgang angenommen wurde.
Die Firmware liest anschließend, wenn verfügbar, einmal die Power-Characteristic:

| Gelesener Wert | Anzeige |
| --- | --- |
| `0x00` | SLEEP |
| `0x01`, `0x08`, `0x09` | Startet |
| `0x0b` | ON |
| `0x02` | STANDBY (nur gelesen; wird niemals gesendet) |
| Sonstige / keine Messung | Unbekannt |

Ein Start kann länger als die erste Messung dauern. Deshalb kann „Startet“
stehen bleiben, obwohl die Station inzwischen läuft. Das Alter der Messung wird
angezeigt; es gibt **keine laufende BLE-Zustandsüberwachung**. SteamVR oder andere
Controller können den Zustand unabhängig verändern. Das Webinterface fragt alle
2,5 Sekunden den ESP32-Speicher ab, nicht die Basisstationen.

## Lokale API / spätere Pebble-Anbindung

Siehe [API.md](docs/API.md). Ein Pebble-Plugin kann später dieselben Endpunkte
aufrufen, mit LocalNetwork-Berechtigung und gespeichertem Schlüssel. Das Plugin
selbst ist noch nicht Teil dieses Prototyps.

Die Verwaltungsoberfläche und `/api/status`, `/api/power`, `/api/scan`,
`/api/config`, `/api/wifi` sind geschützt: HTTP Basic (`admin` + Schlüssel) oder
`X-API-Key`. Der Schlüssel wird nicht in HTML oder Statusantworten ausgeliefert.
Für die Alexa-Emulation bleiben deren Hue-Endpunkte ohne Anmeldung erreichbar;
Geräte im Heimnetz können darüber ebenfalls Schaltbefehle senden. HTTP ist
unverschlüsselt. Deshalb nur im eigenen vertrauenswürdigen Heimnetz betreiben,
keine Portweiterleitung ins Internet einrichten. WLAN-Daten, Schlüssel und
Stationszuordnung liegen im normalen, nicht verschlüsselten NVS des ESP32.

## Selbst bauen

In VS Code die PlatformIO-Erweiterung installieren und diesen Projektordner
öffnen. Profil `esp32dev` verwenden, Build und Upload starten. Mit der CLI:

```sh
pio run
pio run -t upload --upload-port COM5
pio device monitor --port COM5 --baud 115200
```

Die Partitionstabelle sieht zwei je 1,875 MiB große Anwendungsslots vor,
OTA-Updates sind in diesem Prototyp aber **nicht implementiert**. Updates erfolgen
per USB. Webinterface und Skripte sind direkt in der Firmware; kein separater
Dateisystem-Upload und keine externen Webdienste nötig.

Hosttests und Browserprüfung:

```sh
g++ -std=c++11 -Wall -Wextra -Werror -I include test/test_core.cpp -o /tmp/lh-test
/tmp/lh-test
npm install
# Chromium installieren oder CHROMIUM_PATH setzen:
npm test
```

Der Browsertest verwendet simulierte API-Antworten und erzeugt Screenshots unter
`docs/`. Er ersetzt keinen Test auf dem Board. Das geprüfte BLE-Protokoll stammt
aus [Lighthouse PM](https://github.com/jeroen1602/lighthouse_pm/blob/1b1433d124ab3852c4aad62a8265a96a4997d7d0/lighthouse_provider/lighthouse_providers/lib/src/lighthouse_v2/device/lighthouse_v2_device.dart).
Siehe [Quellen und Bibliotheken](docs/SOURCES.md).
