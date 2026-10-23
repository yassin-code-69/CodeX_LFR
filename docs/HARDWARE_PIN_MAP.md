# Hardware pin map and lock (unchanged from V3.3 `main` through V3.5.2)

Verified with `git diff main -- LFR_PRO_V3` : no line defining a pin, button, MUX table, PWM frequency or storage layout changed.

| Function | V3.3 `main` | V3.4 / V3.5 | Changed |
|---|---|---|---|
| MUX S0 / S1 / S2 / S3 | PA7 / PA6 / PA4 / PA3 | PA7 / PA6 / PA4 / PA3 | no |
| Sensor analog in | PA2 (ADC1 IN2) | PA2 (ADC1 IN2) | no |
| Left motor (BTS7960) | PB9 / PB8 | PB9 / PB8 | no |
| Right motor (BTS7960) | PA0 / PA1 | PA0 / PA1 | no |
| PWM | 10 kHz, 8 bit | 10 kHz, 8 bit | no |
| UP / DOWN / ENTER | PB13 / PB12 / PB5 | PB13 / PB12 / PB5 | no |
| BACK = E-STOP | PB3 (raw-polled before every motor update) | PB3, same | no |
| Buzzer / LED | PB4 / PC13 | PB4 / PC13 | no |
| OLED | SSD1306 I2C 0x3C, core default Wire pins | same | no |
| Serial telemetry | Serial1 115200 (PA9/PA10) | same; CSV only if `LFR_RACE_TELEMETRY 1` | no |
| Persistent settings | STM32 internal flash EEPROM emulation, slots 512/768, legacy 256 | same record format | no |

No component is added, removed or replaced. No external EEPROM, encoder or new connector is used.
All four buttons keep their functions; V3.5 adds one menu entry (item 16, TURN MODE) at the end of the existing list.

**Competition note (unresolved, not a firmware issue):** the reported rule of at most two switches including power/reset conflicts with the four fixed buttons. Confirm with the organiser; firmware cannot change the physical switch count and the buttons were deliberately not changed.
