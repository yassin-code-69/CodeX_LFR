# Build and flash (Arduino IDE 2.x)

The sketch is `LFR_PRO_V3/LFR_PRO_V3.ino`. All 13 other firmware files (`LFR_*.h/.cpp`) sit in the same folder and are compiled automatically; do not move or rename them. No external Arduino libraries are needed (only the core's `Wire` and `EEPROM`).

## 1. One-time setup

1. Arduino IDE 2.x -> **File > Preferences > Additional boards manager URLs**:
   `https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json`
2. **Tools > Board > Boards Manager**, search **STM32 MCU based boards** (STMicroelectronics), install version **3.0.0** (the version this firmware was built and tested with).
3. For SWD/Serial upload install **STM32CubeProgrammer** (ST); the IDE calls it.

## 2. Open and select the board

1. **File > Open** -> `LFR_PRO_V3/LFR_PRO_V3.ino`.
2. **Tools > Board > STM32 MCU based boards > Generic STM32F1 series**.
3. **Tools > Board part number > BluePill F103C8**.
4. Leave the other Tools options at their defaults (U(S)ART support: *Enabled (generic 'Serial')*, USB support: *None*, Optimize: *Smallest (-Os default)*). Those are the settings the reference builds used.

## 3. Compile RACE_LOCKED (default)

`LFR_PRO_V3/LFR_Config.h` contains `#define LFR_ALLOW_RACE_START 0`. Press **Verify**. Expected (STM32 core 3.0.0): about **48,780 bytes flash (74%)**, **4,088 bytes RAM**, no warnings. In this build RACE / ARM only shows `RACE LOCKED`; MOTOR CHECK and AUTO calibration can still turn the wheels.

## 4. Compile RACE_UNLOCKED (only when explicitly needed)

1. After the bench checks in `TRACK_TEST_PLAN.md`, change that line to `#define LFR_ALLOW_RACE_START 1` **in your local copy only**.
2. **Verify** (about 49,764 bytes) and **Upload**.
3. Change it back to `0` before committing anything. The repository default must stay race-locked.

Command line equivalent (no file edit):
```sh
arduino-cli compile -b STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8 --warnings all LFR_PRO_V3
arduino-cli compile -b STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8 --warnings all \
  --build-property "compiler.cpp.extra_flags=-DLFR_ALLOW_RACE_START=1" LFR_PRO_V3
```
Other compile switches (`LFR_FAST_SENSOR`, `LFR_RACE_TELEMETRY`, ...) are listed in `CONFIG_PARAMETERS.md`.

## 5. Flash the robot

Use the programming method you already use for this Blue Pill. **Tools > Upload method** offers, among others:

| Method | What it needs |
|---|---|
| STM32CubeProgrammer (SWD) | an ST-Link (or compatible) on the SWD pins SWDIO/SWCLK/GND/3V3; the firmware keeps SWD enabled (only JTAG is disabled to free PB3/PB4) |
| STM32CubeProgrammer (Serial) | USB-UART on PA9 (TX) / PA10 (RX), BOOT0 jumper = 1 while uploading, then back to 0 |
| Maple DFU / HID bootloader | only if that bootloader is already installed on the board |

Before uploading: lift the wheels, have the motor power switch in reach. After uploading: power-cycle and confirm the menu appears with no wheel motion and TURN MODE = CLASSIC.

## 6. Prebuilt binaries

Verified binaries are kept outside the repository in `CodeX_LFR_RELEASES/V3.5.2_905389c/firmware/` (`GENGRIX_X1_V3.5.2_RACE_LOCKED.bin/.hex`, `..._RACE_UNLOCKED.bin/.hex`) with SHA256 sums. Build outputs are never committed (`.gitignore`).
