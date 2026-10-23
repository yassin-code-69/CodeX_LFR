# GENGRIX-X1 line-following robot firmware

Active firmware branch: **`firmware/gengrix-x1-active`** (baseline: V3.5.2, commit `905389c`).

> **Status:** host-tested and compiled for the real target, **not yet tested on the robot**. No speed, stability, safety or competition-readiness claim is made. Firmware ships **race-locked** by default.

## Hardware (fixed - do not change)

STM32F103C8T6 Blue Pill (STM32duino core 3.0.0) · 14 analog IR sensors through a 16-channel MUX · two BTS7960 motor drivers · two 25GA 12 V ~1000 rpm gear motors (no encoders) · SSD1306 OLED · four buttons · 3S LiPo · 510-530 g with battery.

| Function | Pin(s) |
|---|---|
| MUX S0 / S1 / S2 / S3 | PA7 / PA6 / PA4 / PA3 |
| Sensor analog input | PA2 (ADC1 channel 2) |
| Left motor (BTS7960) | PB9 / PB8 |
| Right motor (BTS7960) | PA0 / PA1 |
| UP / DOWN / ENTER | PB13 / PB12 / PB5 |
| BACK = emergency stop | PB3 |
| Buzzer / LED | PB4 / PC13 |
| Serial (debug, telemetry) | Serial1 PA9 TX / PA10 RX, 115200 |

PWM 10 kHz, 8-bit. Settings live in the STM32's internal flash (EEPROM emulation); there is no external EEPROM. Details: [`docs/HARDWARE_PIN_MAP.md`](docs/HARDWARE_PIN_MAP.md).

## Repository layout

```
LFR_PRO_V3/          the Arduino sketch (open LFR_PRO_V3.ino); all 14 firmware files
  LFR_PRO_V3.ino       UI, menus, race loop
  LFR_Config.h         pins, compile-time switches, settings, profiles
  LFR_Sensor.*         MUX + direct ADC1 scan, calibration, line/junction features
  LFR_Control.*        race state machine (follow, gap, pivot, junctions, marker, FAST turn)
  LFR_PID.*            PID with filtered derivative and anti-windup
  LFR_Motor.*          BTS7960 output, slew limiting, PWM cap
  LFR_Storage.*        flash-emulated settings with CRC, dual slot, legacy migration
  LFR_OLED.*           small SSD1306 driver
tests/               host regression suites (g++, mocked Arduino/STM32), see tests/README_TESTS.md
docs/                build/flash, calibration, configuration, architecture, limitations, test plan
CHANGELOG.md         version history
```

## Quick start

1. **Build and flash:** [`docs/BUILD_AND_FLASH.md`](docs/BUILD_AND_FLASH.md) (Arduino IDE 2.x, board BluePill F103C8, STM32 core 3.0.0; RACE_LOCKED by default).
2. **Calibrate:** [`docs/CALIBRATION.md`](docs/CALIBRATION.md).
3. **Test on the bench, then the track:** [`docs/TRACK_TEST_PLAN.md`](docs/TRACK_TEST_PLAN.md) - SAFE profile and TURN MODE CLASSIC first.
4. **Know the limits:** [`docs/KNOWN_LIMITATIONS.md`](docs/KNOWN_LIMITATIONS.md), [`docs/TESTING_STATUS.md`](docs/TESTING_STATUS.md).

## Host tests

```sh
cd tests      # run in a copy or keep tests/previews/ ignored (generated)
sh run_tests.sh && sh run_v32_regressions.sh && sh run_v33_tests.sh && \
sh run_v34_tests.sh && sh run_v35_tests.sh && sh run_v352_tests.sh
```
Expected: 69 `PASS` lines, every script exits 0. Requires `g++` (Linux, WSL, Git Bash with MinGW). These are simulations, not hardware tests.

## Development rules

See [`docs/DEVELOPMENT_WORKFLOW.md`](docs/DEVELOPMENT_WORKFLOW.md). In short: develop on a feature branch from `firmware/gengrix-x1-active`, keep hardware/pins/buttons/flash format unchanged, keep `LFR_ALLOW_RACE_START 0` in commits, run host tests and both STM32 builds before integrating, never commit build outputs, logs or archives, and never touch `main` without explicit approval.

## বাংলা সংক্ষেপ

Arduino IDE 2.x-এ `LFR_PRO_V3/LFR_PRO_V3.ino` খোলো, board **BluePill F103C8** (STM32 core 3.0.0) বেছে নাও, Verify করো। প্রথমে race-locked firmware দিয়ে চাকা তুলে bench test, তারপর calibration, তারপর SAFE + CLASSIC দিয়ে track test। Robot-এ এখনো কোনো test হয়নি।
