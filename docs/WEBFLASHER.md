# GitHub Pages / WebFlasher

Öffentliche Adresse: **https://traumlewis.github.io/Lighthouse-Bridge/**

Die Seite basiert auf [ESP Web Tools](https://esphome.github.io/esp-web-tools/)
10.4.0. Die Module liegen mit veröffentlichten SHA-256-Integritätswerten im
Repository unter `site/vendor/`; zur Laufzeit ist kein CDN nötig.

## Benutzung

1. Die Seite in Chrome oder Edge am Rechner öffnen, ESP32 per USB-Datenkabel
   anschließen und die Board-Bestätigung anklicken.
2. „Verbinden & installieren“ wählen, seriellen USB-Port auswählen und im
   ESP-Web-Tools-Dialog die Installation bestätigen.
3. Bei einer Erstinstallation auf einem zuvor anders genutzten Board den
   Flash-Speicher löschen lassen. Beim normalen Bridge-Update die Löschoption
   **abwählen**, damit NVS und Einstellungen erhalten bleiben.
4. Nach dem Flashen die serielle Konsole / „Logs & Console“ öffnen und
   EN/RESET drücken. Setup-WLAN und individueller Schlüssel erscheinen bei
   115200 Baud. Falls nötig den Flasher erneut verbinden, bevor die Konsole
   geöffnet wird. Ein anderes serielles Programm muss währenddessen geschlossen
   bleiben.
5. Mit dem Setup-WLAN verbinden und `http://192.168.4.1` öffnen; danach der
   [Hauptanleitung](../README.md) folgen.

Ein echter Flashvorgang ist nur mit einem angeschlossenen Board testbar.
Die Firmware unterstützt derzeit kein Improv Serial. Deshalb wird die
Improv-Erkennung im Manifest deaktiviert; das WLAN wird separat im ESP32-Portal
konfiguriert. Es wird kein WLAN-Passwort an GitHub übermittelt.

## Erstmalig GitHub Pages aktivieren

Im Repository **Settings → Pages → Build and deployment → Source → GitHub Actions**
wählen. Das Repository ist bereits öffentlich; seine Sichtbarkeit wird durch den
Workflow nicht geändert. Anschließend bei Bedarf unter **Actions → Firmware &
WebFlasher → Run workflow** den Workflow starten.

Die Aktivierung braucht Administrationszugriff auf Pages. Ein Connector mit
reinen Inhaltsrechten kann die Quelldateien hochladen, aber diese Einstellung
nicht zwingend selbst setzen. Alle weiteren Deployments übernimmt der Workflow.

## Build und Deployment

`.github/workflows/firmware-pages.yml` baut bei jedem Push auf `main`:

1. Native Tests der ON/SLEEP-Auftragslogik.
2. Die Firmware per PlatformIO mit den festgelegten Abhängigkeiten.
3. Ein zusammengeführtes ESP32-Image und das Quellcode-ZIP.
4. Das statische Pages-Verzeichnis inklusive Manifest, Firmware und Prüfsumme.
5. Prüfungen für relative Pfade, Lazy-Loading-Module und Firmware-Integrität.
6. Ein Pages-Deployment über die offiziellen GitHub-Actions.

Pull Requests werden gebaut und geprüft, aber nicht veröffentlicht. Fertige
Firmwarepakete stehen zusätzlich als Actions-Artefakt `lighthouse-bridge-firmware`
bereit. Das Pages-Manifest enthält ausschließlich `chipFamily: ESP32`, ein
zusammengeführtes Image mit `offset: 0` und die Lösch-Rückfrage. Eine neue
Firmware wird nur nach einem erfolgreichen Build und den Prüfungen ausgerollt.

Lokaler Build (nach `pio run`):

```sh
python tools/package_release.py
python tools/build_pages.py
python -m unittest discover -s test -p 'test_pages.py' -v
python -m http.server 8080 --directory _site
```

Unter `http://localhost:8080` ist Web Serial ebenfalls erlaubt. Für entfernte
Seiten ist HTTPS erforderlich, das GitHub Pages bereitstellt. Das Verzeichnis
`_site/` ist generiert und wird nicht eingecheckt; `site/` enthält die Quelldateien.
Sämtliche URLs sind relativ, damit die Projekt-Unteradresse `/Lighthouse-Bridge/`
funktioniert. Für eine andere Repository-Adresse die Links in README und Webseite
entsprechend anpassen.

Zur Versionierung aktuell `VERSION` in `tools/package_release.py` anpassen.
Das Pages-Manifest und die Downloadpakete übernehmen diese Version automatisch;
die Firmware-Version in `src/main.cpp` und `include/web_ui.h` ebenfalls mitziehen.
