# Prüfbericht — Prototyp 0.1.0

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

Keine dieser Hardwareprüfungen wird durch einen erfolgreichen Build oder den
Browser mit simuliertem Backend nachgewiesen. Die Firmware wurde hier nicht auf
ein Board geflasht und hat keine realen Basisstationen oder Echos angesprochen.
