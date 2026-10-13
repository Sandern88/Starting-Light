# F1 Start Light (ESP32-S2 Mini)

Dette projekt styrer et Formel 1 inspireret startlys med:

- ESP32-S2 Mini
- 8-kanals relae board
- 5 rode 12V lys
- PIR-sensor som trigger
- Lyd (buzzer via relae) under hele nedtaellingen
- Simpelt webpanel til status og manuel styring

## Adfaerd

- Systemet starter i ARMED_WAIT_PIR
- PIR trigger starter countdown
- Rod 1->5 taender med interval
- Lyd er ON under countdown
- Efter sidste rode lys slukker alle rode samlet
- PIR ignoreres under aktiv sekvens
- Efter lockout er systemet klar igen

## Webpanel

Enheden koerer som Access Point:

- SSID: F1StartLight
- Password: startlight123
- URL: http://192.168.4.1/

Panel har knapper til:

- Arm
- Disarm
- Start Test
- Abort
- Reset Fault

Live controls i webpanel:

- Red step ms
- All off delay ms
- Complete ms
- Lockout ms
- Save Timings

Hardware test i webpanel:

- Run Hardware Test (tester Red1, Red2, Red3, Red4, Red5, Buzzer i raekke)
- Stop Hardware Test
- Step ms (hvor laenge hver kanal er aktiv)

API endpoints:

- POST /api/timings?redStepMs=1000&allOffDelayMs=300&completeStateMs=700&lockoutMs=5000
- POST /api/hwtest/run?stepMs=700
- POST /api/hwtest/stop

## Build og flash

1. Installer PlatformIO i VS Code
2. Tilslut ESP32-S2 Mini via USB
3. Koer build/upload via PlatformIO tasks

## Vigtigt om hardware

- Brug relae modul med driver/transistor (ingen direkte relae coil fra GPIO)
- Bekraeft om dit relae modul er active LOW eller active HIGH
- 12V lasten skal forsynes korrekt og sikkert
- Faelles reference/jord skal vaere korrekt efter dit relae modul design

Se wiring-noter i docs/WIRING.md.
