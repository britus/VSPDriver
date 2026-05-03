# VSPClient Endanwender-Dokumentation

## Überblick

VSPClient ist die grafische Steuerzentrale für den **Virtual Serial Port Controller** unter macOS. Die App erstellt virtuelle serielle Ports, verbindet diese zu Port-Links, zeichnet Treiberaktivitäten auf und öffnet Terminalfenster zum Testen von Datenströmen.

Die Anwendung richtet sich an Anwender, die serielle Schnittstellen simulieren, testen oder zwischen mehreren Tools und Geräten logisch verschalten möchten.

## Hauptbereiche der App

Die Seitenleiste gliedert die App in drei Arbeitsbereiche:

### 1. Serial Ports

Im Bereich **Serial Ports** verwalten Sie die vorhandenen virtuellen Schnittstellen.

- Die Liste links zeigt alle angelegten virtuellen Ports.
- Rechts sehen Sie die Details des aktuell markierten Ports.
- Über `+` legen Sie einen neuen Port an.
- Über das Papierkorb-Symbol löschen Sie den ausgewählten Port.

Typische Portparameter:

- Baudrate
- Datenbits
- Stoppbits
- Parität
- Flow Control

Ein neuer Port wird über den Dialog **Create Serial Port** angelegt. Dort wählen Sie die gewünschten Kommunikationsparameter aus.

## 2. Port Links

Im Bereich **Port Links** verbinden Sie virtuelle Ports zu logischen Kabeln.

Ein Link besteht aus:

- einem **Source Port**
- einem **Target Port**
- optional einem **Target 2**

### Linktypen

#### Punkt-zu-Punkt

Ein einfacher Link verbindet genau zwei Ports miteinander.

Beispiel:

- `vsp1 <-> vsp2`

#### Verzweigter Link mit zwei Zielen

Ein Link kann zusätzlich ein zweites Ziel enthalten.

Dabei stehen zwei Routing-Modi zur Verfügung:

- **Answer Source only**  
  Antworten von Zielports gehen nur an den Quellport zurück.
- **Answer Source and peer target**  
  Antworten werden an den Quellport und an den jeweils anderen Zielport weitergeleitet.

### Link anlegen

1. Öffnen Sie **Port Links**.
2. Klicken Sie auf `+`.
3. Wählen Sie **Source port** und **Target port**.
4. Optional wählen Sie **Target 2**.
5. Aktivieren Sie bei Bedarf **Answer source only**.
6. Klicken Sie auf **Create**.

Hinweis: Ein neuer Link kann nur erstellt werden, wenn mindestens zwei freie, noch nicht verlinkte Ports verfügbar sind.

## 3. Driver Traces

Im Bereich **Driver Traces** legen Sie fest, welche Treiberprüfungen und Datenflüsse protokolliert werden.


Aktivierbare Prüfungen:

- Baud rate
- Data bits
- Stop bits
- Parity
- Flow control

Aktivierbare Traces:

- Trace RX
- Trace TX
- Trace control

Verfügbare Aktionen:

- **Save To Driver** speichert die aktuelle Trace-Konfiguration.
- **Refresh** lädt den aktuellen Treiberstatus neu.
- **Clear Log** leert die Logausgabe.

Der untere Bereich zeigt das Treiberprotokoll in Echtzeit an.

## Terminalfenster öffnen

Über das Terminal-Symbol in der oberen Werkzeugleiste öffnen Sie ein neues **Serial Terminal**.

Ein Terminal dient zum direkten Test von Datenverkehr auf einem virtuellen Port.

## Arbeiten mit dem Serial Terminal

Im Terminal wählen Sie zunächst:

- den seriellen Port
- Baudrate
- Datenbits
- Stoppbits
- Parität
- Flow Control

Danach verbinden Sie sich über das Verbindungs-Symbol.

### Text senden

- Tragen Sie den ausgehenden Text in **Text to send** ein.
- Aktivieren Sie bei Bedarf **Add CR** und **Add LF**.
- Senden Sie den Inhalt über die Senden-Funktion.

### Dateiinhalte senden

Über das Symbol **Send File** können Sie den Inhalt einer Datei in Blöcken an den verbundenen Port übertragen.

### Loop-Test

Mit der Loop-Funktion erzeugt das Terminal automatisch fortlaufende Teststrings.

- **Loop length** bestimmt die Länge des erzeugten Textes.
- Starten und stoppen Sie den Test über das Loop-Symbol.

### Loganzeige

Der untere Bereich des Fensters zeigt:

- gesendete Daten
- empfangene Daten
- Statusmeldungen

## Typische Test-Szenarien

### Zwei Ports direkt gegeneinander testen

1. Legen Sie zwei virtuelle Ports an.
2. Erstellen Sie einen Link zwischen beiden Ports.
3. Öffnen Sie zwei Terminalfenster.
4. Verbinden Sie jedes Terminal mit einem anderen Port.
5. Senden Sie Testdaten in einem Fenster und prüfen Sie die Ausgabe im anderen.

## Treiberinstallation und Freigabe

Wenn der Treiber noch nicht vollständig aktiviert ist, blendet die App eine Hinweisseite ein.

Mögliche Zustände:

- Warten auf Aktivierung der Driver Extension in macOS
- Neustart erforderlich

Folgen Sie in diesem Fall den Anweisungen der App, aktivieren Sie die Driver Extension in den Systemeinstellungen und starten Sie den Mac gegebenenfalls neu.

## Tipps für den Alltag

- Löschen Sie einen Port nur dann, wenn Sie den dazugehörigen Link nicht mehr benötigen.
- Verwenden Sie **Driver Traces**, wenn eine Verbindung unerwartet reagiert oder Parameter nicht korrekt übernommen werden.
- Nutzen Sie das Terminal für Funktionsprüfungen, bevor Sie externe Software oder Hardware anbinden.
- Für komplexere Routingszenarien lohnt sich ein Link mit zweitem Zielport.
