# H-Guard 

## Beschreibung
Dieser Code wurde speziell für die H-Guard-Komponente des Projekts HomeWatch entwickelt. 

Das H-Guard-Modul wird an der Eingangstür von Wohnungen oder Häusern montiert und bietet folgende Funktionen:

ANMERKUNG: ESP32 arbeitet im Multithreading-Modus!

Bewegungserkennung: Der Sensor erkennt Bewegungen im Eingangsbereich.
Fotoaufnahme: Bei erkannten Bewegungen wird automatisch ein Foto geschossen.
Live-Bilder: Es werden Livebilder angezeigt. 
Alarmierung: Bei ausgewählten Ereignissen wird ein Alarm ausgelöst.
Die aufgenommenen Fotos werden in der Firebase-Datenbank gespeichert.

## Anforderungen
- Freenove ESP32 Wrover
- OV2640 Kamera
- Sharp IR-Sensor GP2Y0A02YK0F
- SSD1306 OLED-Display



![H-Guard](https://github.com/DevWuchte/H-Guard/blob/main/H-Guard%20Proto.jpg)



