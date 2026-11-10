# GitHub Pages / WebFlasher — 0.2.0

**https://traumlewis.github.io/Lighthouse-Bridge/**

ESP Web Tools 10.4.0 liegt unverändert unter `site/vendor/`, inklusive aller
Lazy-Loading-Module und veröffentlichter Integritätswerte in `UPSTREAM.json`.
Zur Laufzeit wird kein CDN benötigt. HTTPS kommt von GitHub Pages; lokal ist
`http://localhost` zulässig. Chrome/Edge auf einem Rechner verwenden.

## Sprachen

Englisch ist Standard. Der Schalter bietet Deutsch; Auswahl unter
`lighthouse-bridge.language` im lokalen Browserspeicher. `?lang=en` / `?lang=de`
hat Vorrang vor der gespeicherten Auswahl, danach Standard Englisch. Gesperrter
Browserspeicher verhindert den Sprachwechsel nicht. `site/translations.js`
enthält Seitentexte, Status-/Fehlermeldungen, Metadaten und Beschriftungen.
Sprachwechsel setzt keine Checkboxen oder USB-Verbindungen zurück.

Das ESP32-Webinterface ist nun ebenfalls EN/DE mit derselben Auswahlregel.
Der unveränderte ESP-Web-Tools-Dialog und die USB-Port-Auswahl des Browsers haben
weiterhin ihre eigenen Spracheinstellungen und können Englisch bleiben.

## Updates mit erhaltenen Einstellungen

**Ab 0.2.0 verwendet der Flasher vier getrennte Teile:**

| Datei | Offset |
| --- | --- |
| `lighthouse-bridge-bootloader.bin` | `0x1000` |
| `lighthouse-bridge-partitions.bin` | `0x8000` |
| `lighthouse-bridge-boot-app0.bin` | `0xe000` |
| `lighthouse-bridge-app.bin` | `0x10000` |

Der NVS-Bereich `0x9000..0xdfff` wird nicht geschrieben oder durch angrenzende
Flashsektoren gelöscht. `Erase device` muss beim Update abgewählt bleiben.
`new_install_prompt_erase: true` ermöglicht diese Auswahl.

**Fehler in 0.1.x:** Das damalige zusammengeführte Image an `0x0` schrieb seine
Füllbytes über NVS. Die frühere Aussage, Erase abzuwählen reiche für dieses Image,
war falsch. Der Factory-BIN-Download bleibt für Neuinstallationen verfügbar und
ist entsprechend beschriftet. Für Updates den WebFlasher oder die vier Teile
mit ihren jeweiligen Offsets verwenden. Die Speicheraufteilung bleibt gleich.

`tools/package_release.py` erzeugt Factory-Image und sichere Teilimages und prüft
sie gegeneinander. `tools/build_pages.py` kopiert diese Dateien nach `_site/`,
erzeugt das Vier-Teile-Manifest und prüft, dass die Sektorbereiche NVS aussparen.
`test/test_pages.py` simuliert die sektorweisen Erase-/Write-Operationen mit
vorhandenem NVS-Inhalt. `site/flash-layout.mjs` lehnt unsichere Manifeste vor dem
Freischalten des Installationsknopfs ab.

Die Teilimage-URLs enthalten ihre SHA-256-Prüfsumme als Query, damit der Browser
beim nächsten Update keine ältere Datei aus dem Cache verwendet.

## Schlüssel und WLAN-Adresse über USB

1. Nach der Installation den ESP-Web-Tools-Dialog schließen.
2. Die Seite übernimmt dessen bereits freigegebenen Port und startet den
   USB-Infobereich. Alternativ „Read key & address“ drücken und den Port wählen.
3. API-Key und Setup-WLAN erscheinen, sobald die Firmware antwortet. USB verbunden
   lassen; nach der WLAN-Einrichtung erscheint auch die lokale Webadresse.
4. Vor erneutem Flashen „Disconnect USB“ drücken. Ein serieller Port kann immer
   nur von einem Leser genutzt werden. Andere Konsolen/Programme schließen.

Die aktuelle Firmware implementiert `LH_INFO\n` bei 115200 Baud. Solange der
Leser aktiv ist, fragt `site/serial-info.mjs` alle zwei Sekunden Informationen ab.
Die erste Antwort darf nach Boot/Verbindungsversuchen bis zu 45 Sekunden dauern.
Die Firmware bearbeitet USB-Befehle auch während des nicht blockierenden
WLAN-Setup-Portals. Alte Firmware ohne dieses Protokoll muss zuerst aktualisiert
werden. Die Adresse wird validiert; nur eine einfache HTTP-IPv4-Adresse mit Pfad
`/` wird als Link angeboten. Schlüssel werden weder in localStorage, URLs oder
Cookies abgelegt noch an GitHub übertragen. Das eigene Web-Passwort erscheint
weder in USB-Infos noch im WebFlasher.

Der ESP-Web-Tools-Dialog behält den Port bis zum Schließen. Deshalb übernimmt die
Seite erst nach dessen `closed`-Ereignis; sie greift nicht während des Flashens in
seinen Datenstrom ein. Nach dem Schließen kann man aufgeteilte Boot-Zeilen im
USB-Leser empfangen; der Parser ignoriert andere Logs und ungültige Frames.
Improv Serial ist weiterhin **nicht implementiert**. WLAN wird über das ESP32-
Portal eingerichtet, nicht direkt im ESP-Web-Tools-Dialog.

## Build und Veröffentlichung

Pages ist bereits auf **GitHub Actions** eingestellt. Ein Push auf `main` führt
native Tests, Firmware-Build, Paketierung, Seiten-/NVS- und USB-Prüfungen aus und
veröffentlicht bei Erfolg. Pull Requests werden geprüft, nicht veröffentlicht.
Artefakte: `lighthouse-bridge-firmware` und `lighthouse-bridge-debug` (ELF/map).

```sh
pio run
python tools/package_release.py
python tools/build_pages.py
python -m unittest discover -s test -p 'test_pages.py' -v
node test/serial_info.cjs
npm test
python -m http.server 8080 --directory _site
```

`site/` ist Quelle, `_site/` generiert und nicht eingecheckt. Alle Pfade müssen
unter `/Lighthouse-Bridge/` funktionieren. Browserprüfungen verwenden simulierte
Geräte und ersetzen keinen echten Flashvorgang. Die Firmware-Version steht
aktuell in `src/main.cpp`, `include/web_ui.h` und `tools/package_release.py`;
Versionsänderungen dort und in Dokumentation/AGENTS gemeinsam durchführen.
