# Wiring Notes (ESP32-S2 Mini + 8CH Relay + PIR + 12V Lights)

## IO mapping (default)

- PIR input: GPIO7
- Relay IN1 (Red 1): GPIO1
- Relay IN2 (Red 2): GPIO2
- Relay IN3 (Red 3): GPIO3
- Relay IN4 (Red 4): GPIO4
- Relay IN5 (Red 5): GPIO5
- Relay IN6 (Buzzer): GPIO6

IN7 og IN8 er reserveret til fremtidige outputs.

Pinmap kan aendres i src/Config.h.

## Signal flow

- ESP32 GPIO -> relay input
- Relay contact -> bryder/slutter 12V til hver lampe
- PIR OUT -> ESP32 PIR pin

## Safety

- Koer aldrig relae coil direkte fra ESP32 GPIO
- Brug relae board med transistor/optokobler
- Brug sikring paa 12V-linje til lampelast
- Hold lavspaending og lastkabler fysisk adskilt

## First power-on checklist

1. Flash firmware med lamper frakoblet
2. Verificer at alle relae er OFF ved boot
3. Test webpanel og state-skift
4. Tilslut en lampe ad gangen
5. Koer 10+ sekvenser og check stabilitet

## Hardware test mode

- Start fra webpanel: Run Hardware Test
- Firmware aktiverer en kanal ad gangen i denne raekke:
	- Red1 -> Red2 -> Red3 -> Red4 -> Red5 -> Buzzer
- Hver kanal holdes aktiv i Step ms
- Stop fra webpanel: Stop Hardware Test
