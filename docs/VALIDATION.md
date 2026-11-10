# Prüfbericht — Prototyp 0.2.0


## Rückmeldung zu 0.1.1 und Erweiterungen 0.2.0

Der Besitzer bestätigt erfolgreichen Start und grundlegende Funktion mit 0.1.1.
Gemeldet wurden verlorene WLAN-Einstellungen beim Update sowie nicht gemeinsam
aktualisierte Zustände von Webinterface/Alexa. Die Ursache der Update-Verluste
ist im Imageformat nachgewiesen: Das zusammengeführte Image enthält Füllbytes
über NVS. Die frühere Empfehlung „nicht löschen“ reichte damit nicht aus.

0.2.0 verwendet vier getrennte Flashteile. Ein Regressionstest simuliert die
sektorweisen Erase-/Write-Operationen über bestehendem NVS-Inhalt und bestätigt,
dass `0x9000..0xdfff` unverändert bleibt; das Factory-Image reproduziert die alte
Überschreibung. Teilimages stimmen byteweise mit dem gepatchten Image überein.

Die Firmware liest Zustände beim Start, regelmäßig und manuell ohne Schreibbefehl.
Native Tests prüfen gemessene Werte statt Zielwerten, Fehler/Staleness und
Millisekunden-Überlauf. Hue-GETs liefern dieselben Daten, aber können keinen
Status-Push in die Alexa-App erzwingen.

Browserprüfungen mit simuliertem Backend/USB bestanden: eigener Passwort-Login
nach API-Key-Bootstrap, verpflichtende Passwortvergabe, API-Key nicht mehr als
späteres Login, Key im angemeldeten Netzwerk-Reiter, Logout, EN/DE und mobile
Ansichten, Zustand vor erstem Schaltbefehl, keine optimistische Messwert-Erfindung,
aktualisierte und veraltete Messungen. USB-Tests prüfen echten Stream-Parser,
geteilte Frames, ungültige URLs, WLAN-Adresswechsel, Trennen und Übergabe nach
Schließen des Installationsdialogs. Schlüssel werden nicht gespeichert/übertragen.

Build, native Tests, Python-Layout/Integritätsprüfungen und Node-USB-Tests bestanden.
**Die erweiterten Funktionen müssen noch am realen Board und Echo geprüft werden.**
Insbesondere sind Passwort-Hashing/Session-Routen im Gerät, WLAN-Portal mit
USB-Abfragen, NVS-Erhalt nach echtem Flash, BLE-Dauerabfragen und Alexas Abfrage-
/Cache-Verhalten nicht durch Browser-Mocks nachgewiesen.

## Hardware-Rückmeldung und Startkorrektur

Am 10.11.2026 meldete der Besitzer den ersten echten Start mit 0.1.0: Firmware
geflasht, Zugangsschlüssel ausgegeben, Heim-WLAN verbunden; anschließend Abbruch
bei der Bluetooth-Initialisierung. Die lokalen Symbole des entsprechenden
Quellstands ordnen den Backtrace `coex_core_enable → coex_enable →
esp_bt_controller_enable → NimBLEDevice::init → setup` zu. Das exakte ELF des
ursprünglichen CI-Builds wurde noch nicht archiviert; künftig bleibt es als
Actions-Artefakt `lighthouse-bridge-debug` verfügbar. Es werden keine persönlichen
Geräte-/Netzwerkdaten oder Zugangsschlüssel aus dem Log im Repository gespeichert.

Ursache: `WiFi.setSleep(false)` setzte `WIFI_PS_NONE`, bevor BLE aktiviert wurde.
Version 0.1.1 setzt stattdessen explizit `WIFI_PS_MIN_MODEM` und prüft den
Treiber-Rückgabewert, bevor BLE startet. Bei einem Fehler wird BLE nicht gestartet.
Auch der von Arduino gespeicherte Modus wird vor dem STA-Start gesetzt, damit
spätere WLAN-Neustarts nicht wieder `WIFI_PS_NONE` anwenden.
Die Bereitschaftsmeldung und Webadresse erscheinen erst nach der Initialisierung.
Diese WLAN-Modem-Einstellung ändert den Lighthouse-SLEEP-Befehl nicht.

Referenz: [Espressif, IDFGH-8094 / Issue 9595](https://github.com/espressif/esp-idf/issues/9595).
Die zusätzliche Meldung zur fehlenden Core-Dump-Partition ist ein separates
Diagnoseproblem: Die bisherige Partitionstabelle enthält keine Core-Dump-Partition.
Für dieses Update bleiben alle Partitionen und gespeicherten Einstellungen
unverändert. Der Besitzer hat den erfolgreichen Start mit **0.1.1** inzwischen bestätigt.

## In der Entwicklungsumgebung geprüft

- PlatformIO-Build: `espressif32@6.9.0`, Profil `esp32dev`, Arduino ESP32 2.0.17.
- Anwendung passt in den 1.875-MiB-Slot. Der erste erfolgreiche Build benötigte
  rund 1,20 MB Flash und 59 KB statisches RAM. Laufzeit-Heap/Task-Stacks sind darin
  nicht enthalten; der echte Speicherbedarf muss auf dem Gerät geprüft werden.
- Native C++-Tests: ausschließlich ON/SLEEP als Eingabebefehle; STANDBY und
  ungültige Werte abgelehnt; MAC-/Namensvalidierung; gelesene Boot-Zustände;
  überholte Aufträge dürfen neuere Aufträge nicht als erledigt markieren;
  fehlgeschlagene Befehle erfinden keine Messwerte.
- Playwright/Chromium: das unveränderte eingebettete HTML mit simulierten
  API-Antworten. Vier zunächst unkonfigurierte Plätze, Gerätesuche, Zuordnung
  einschließlich Random-Adresstyp, Erhalt ungespeicherter Formulare bei Polling,
  Speicherfehler, Einzel-/Gruppenbefehle ON/SLEEP, BLE-Fehleranzeige,
  Verbindungsabbruch/-wiederkehr, Start des WLAN-Setups.
- Layout bei 1280 und 390 Pixel Breite, kein horizontaler Überlauf.
  Screenshots zeigen simulierte Daten, keine echten Stationen.
- Zusammengeführtes Flashimage: Bootloader, Partitionstabelle, OTA-Metadaten
  und Anwendung werden gegen die jeweiligen Bytes an ihren Zieladressen geprüft.
- ZIP-Integrität und SHA-256-Prüfsumme werden bei der Paketierung geprüft.
- WebFlasher: statische Prüfungen für das ESP32-Manifest mit Offset 0, die
  veröffentlichten Firmware-Prüfsummen und die Modul-/Projektpfade bestanden.
  Playwright hat die echten lokal eingebundenen ESP-Web-Tools-Module geladen,
  die Board-Bestätigung und den Aufruf des USB-Port-Dialogs geprüft. Web Serial
  war dabei simuliert; kein echtes USB-Gerät wurde angesprochen. Ein fehlendes
  Firmwareimage deaktiviert Installation und Download. Desktop und mobile Ansicht
  wurden geprüft; Screenshots unter `docs/webflasher-*.png`.
- Zweisprachiger WebFlasher: englische Standardansicht, deutscher Sprachwechsel,
  gespeicherte Präferenz, Vorrang von `?lang=en`, übersetzte Fehler-/Lademeldungen
  und Beschreibungs-Metadaten geprüft. Die Installationsbestätigung bleibt beim
  Wechsel erhalten; beide Sprachfassungen passen in die mobile Ansicht.

## Noch an echter Hardware zu prüfen

1. Modul tatsächlich klassischer ESP32, Flash mindestens 4 MB, CH340-Port und
   Flashvorgang über USB. Erster Start, individueller Schlüssel, NVS-Speicherung.
2. WLAN-Portal auf Handy/PC, falsches WLAN-Passwort und korrigierte Eingabe,
   BOOT-Langdruck nach Start, Portal-Timeout, WLAN-Wechsel/-Wiederverbindung.
3. Geschützte Web-/API-Endpunkte mit korrekten und falschen Anmeldedaten.
4. Suche nach allen vier Lighthouse V2. Stationen anhand der MAC zuordnen;
   zunächst eine Station einzeln prüfen: ON, dann **SLEEP**, nicht STANDBY.
5. Einzel- und Gruppenbefehle mit allen vier Stationen. BLE-Reichweite,
   Wiederholungen und Anzeige bei stromloser/unerreichbarer Station prüfen.
6. Schnelle Folge ON → SLEEP: Der jüngste Auftrag muss zuletzt ausgeführt
   werden. Neustart darf keinen Schaltbefehl auslösen; Zuordnung bleibt erhalten.
7. Echo Dot 5: SSDP-Gerätesuche, fünf Hue-Lampen, Einzel-/Gruppen-Sprachbefehle,
   Namensänderungen. Alexa-Zustand ist absichtlich optimistisch und bestätigt
   keinen BLE-Erfolg.
8. Längerer Betrieb: freier Heap, Web-Erreichbarkeit während BLE-Aufträgen,
   Verhalten parallel zu SteamVR und nach Strom-/WLAN-Unterbrechungen.

Ein erfolgreicher Build oder Browser mit simuliertem Backend ersetzt keine
Hardwareprüfung. Die oben beschriebene Besitzer-Rückmeldung belegt den Start und
die WLAN-Verbindung von 0.1.0, aber keinen erfolgreichen BLE-Start. Die Firmware
wurde hier nicht auf ein Board geflasht; erfolgreiche Basisstations- oder
Echo-Steuerung wurde noch nicht nachgewiesen.
