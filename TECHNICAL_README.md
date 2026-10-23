> **Note (V3.4+):** this file describes the V3.3 foundation. For the current behaviour, constants and test results see `CHANGELOG.md` (and `docs/` on `dev/v3.5-performance`).

# LFR PRO V3.3 - Reliability-Corrected Full Source + Speed Foundation

**Base:** the user's unpushed `LFR_PRO_V3.rar` (14 source files). This is a **conservative software-fix release**, not a hardware-certified or guaranteed winning race firmware. V3.3 adds the speed foundation (direct ADC, single-write EEPROM save, timing measurements, bug fixes) without changing race behaviour; see `CHANGELOG.md`.

## Hardware configuration preserved

- Controller: STM32F103C8T6 Blue Pill, Arduino STM32 official core.
- Sensor: 14 analog channels via 74HC4067-style MUX: PA7/S0, PA6/S1, PA4/S2, PA3/S3, PA2/ADC.
- BTS7960 dual motor driver: Left PB9/PB8; Right PA0/PA1.
- Buttons: PB13 UP, PB12 DOWN, PB5 ENTER, PB3 BACK/E-STOP.
- OLED: SSD1306 I2C, address 0x3C.
- Motors: 2x 12V 25GA 1000RPM nominal DC geared motors, **no encoder feedback established**.
- **Robot mass including battery: 510-530 g**, as provided by the owner. Mass alone does not determine stopping distance, tire grip, torque, PWM deadband, or optimal gains.

## How to open and build

1. Extract the ZIP. **Keep the `LFR_PRO_V3` folder and all its `.ino`, `.cpp`, `.h` files together.**
2. In Arduino IDE, open **`LFR_PRO_V3/LFR_PRO_V3.ino`**, select the correct STM32F103C8T6 Blue Pill variant / STM32duino (STMicroelectronics) board package used for your working build.
3. Compile/Verify and **check the reported Flash and RAM** are within your actual Blue Pill limits (nominal 64KB/20KB). V3.3 was compiled and linked for `BLUEPILL_F103C8` with STMicroelectronics:stm32 3.0.0 (`--warnings all`): **Flash 46,712 B (71%), RAM 4,024 B, zero warnings.** It has not been flashed or run on the robot.
4. Before the first upload, back up the working firmware and settings. Lift both wheels off the floor and have a motor-power cutoff reachable.
5. In `LFR_Config.h`, first boot is **`#define LFR_ALLOW_RACE_START 0`**. This deliberately locks racing; **motor check / auto-calibration can still move motors**. Check PWM direction, 14 sensor channels and PB3 physical E-stop.
6. Calibrate WHITE and BLACK across all 14 channels, SAVE SETTINGS, reboot, verify persistence. Use the **SAFE** profile for initial track trials. Confirm `FINISH=OFF`, `GAP=RECOVER`, `NAV=STRAIGHT` until the official course rules are known.
7. **Only after the above pass**, change `LFR_ALLOW_RACE_START` to **1** and reflash. Start on a slow practice track, not at FAST/SPRINT speeds.
8. Do not automatically copy a previous competition setting into this build without verifying menu settings: EEPROM data layout is intentionally the same, so previous high-speed values can still load.

## What has been fixed or guarded

- All race counters, including turbo state, now reset when `Controller::start()` is called.
- A connected left/right **side fan** across the sensor bar is no longer called a junction solely because 8+ sensors are active. This reduces a known 90-degree-corner/route-step false positive.
- `GAP` re-acquires persistent black pixels **even when the first detected pattern is a junction or marker**; no longer waits forever for `!f.junction`. Gap STOP sends zero immediately to the motor slew logic.
- Black **start-pad** is treated separately from the finish system; an extended black start area times out safely instead of being treated as a finish.
- `FINISH=ON` only accepts an **eligible, opt-in** marker after a clean stable approach, sufficient time and width, and (in scripted navigation) after all route steps. `FINISH=OFF` cannot accidentally call `FINISHED`. An unknown black patch eventually stops instead of driving blind forever.
- Race lap/launch clock starts **after** the expensive OLED `show()` call.
- Alpha-beta estimator retained for A/B tests but disabled by default. The default derivative is bounded and time-constant filtered (less estimator ringing in synthetic tests).
- Motor PWM acceleration/braking steps now scale with actual control-loop time (settings still mean steps per 5ms); forced direction reversal still passes through zero.
- Optional stiction PWM mapping is **off by default** and always physically capped at `maxPWM` when enabled; unmeasured PWM 1-to-20 jumps are not imposed on the robot.
- Normal FOLLOW does not command reverse counter-torque by default (pivot still can); 510-530g robot braking must be tested before enabling aggressive reverse.
- Fast-MUX 16-entry BSRR table verified; MUX settle default increased to 5us with an optional discard-first-ADC compile flag. **Neither fast ADC precision nor 333Hz actual execution is hardware verified.**
- V3.3: `LFR_FAST_SENSOR=1` now really bypasses `analogRead()`. ADC1 is configured and calibrated once; each channel is one register-level conversion (41.5-cycle sampling at 12 MHz). STM32duino's `analogRead()` re-initialised and calibrated ADC1 on every call and overwrote the old sample-time write. A conversion that never completes latches an ADC fault: arming is refused and a race stops.
- V3.3: SAVE SETTINGS performs one flash page erase per save (`EEPROM.put()` erased the page once per changed byte on the real core). AUTO CAL with MIRROR=REVERSED now stores values at the physical sensor index.
- V3.3: SENSOR LIVE shows the measured scan time; the race result shows the worst control-step work time (`LOOP xxxUS`), and serial adds `LOOP_MAX_US`, `DT_MAX_US`, `SCAN_US`.
- Existing UI, storage structure, menu, profiles and core hardware pin mapping preserved.

## Compile-time switch defaults (`LFR_Config.h`)

| Setting | Value | Notes |
|---|---:|---|
| `LFR_ALLOW_RACE_START` | `0` | Set to 1 **after** bench checks; then reflash |
| `LFR_FAST_SENSOR` | `1` | Direct MUX + ADC1 registers; 0 = original `analogRead()` path. Compare raw values on real hardware |
| `LFR_ADC_SAMPLE_CYCLES` | `4` | SMPR code: 41.5 cycles (2=13.5, 3=28.5, 5=55.5) |
| `LFR_SENSOR_DUMMY_READ` | `0` | Optional discard-first-ADC for channel crosstalk comparisons |
| `LFR_PID_USE_ALPHA_BETA` | `0` | Experimental estimator option remains, default is stable lowpass D |
| `LFR_MOTOR_DEADBAND` | `0` | Only enable from measured stiction threshold; final physical duty is capped |
| `LFR_ALLOW_FOLLOW_REVERSE` | `0` | Avoid forced reverse in FOLLOW before grip tests |
| `CTRL_PERIOD_US` | `3000` | Nominal ~333Hz, contingent on sensor scan budget |
| `LFR_FINISH_CONFIRM_MS` | `250` | V3.4: confirmed full-marker dwell; must exceed `LFR_CROSSING_MAX_MS` (200) so a crossing line is never a finish |
| `LFR_CROSSING_MAX_MS` | `200` | V3.4: all-black that clears sooner is a T/+ crossing |
| `LFR_GAP_RECOVER_MS` | `300` | V3.4: blind gap budget (was a fixed 180 ms) |
| `LFR_MARKER_TIMEOUT_MS` | `900` | Safety stop if black area has no usable line for too long |

## Remaining limitations - READ BEFORE COMPETITION

1. **A full-black finish pad and a large full-black intersection can look identical** to a 14-channel reflectance array; software cannot guarantee a correct finish without course semantics. **Leave FINISH OFF until organizer rules and sensor replay tests are available.**
2. An optically **connected T junction** can look like a broad 90-degree corner, so conservative classification can miss some true junctions. Build sensor replay sequences for the actual course and confirm expected route behavior.
3. No wheel encoders, IMU or camera. Absolute traveled distance, real RPM, and reliable shortest-route mapping are unavailable.
4. The 510-530g mass is below 1 kg, but **does not justify exact PWM/gain/pivot values**. Tire material, gearing, chassis geometry, sensor height, battery voltage and flooring dominate tuning.
5. Physical PB3 E-stop, startup/remap, ADC channel settling and STM32 Flash/EEPROM emulation **must** be validated on your board. Flash save power loss protection was not proven in situ. Both EEPROM slots live in the same 1 KB flash page, so a power cut during SAVE can lose both copies (the robot then boots on defaults and asks for calibration).
6. A software E-stop cannot replace a physical motor-power disconnect. Do not run raised-wheel tests with the wheels touching objects or fingers.
7. This ZIP was **not** uploaded to or run on the robot. Do not assume national competition readiness from host test results.

## Host regression tests

On Linux/macOS with `g++`, from the `tests` folder run:

```sh
sh run_tests.sh
sh run_v32_regressions.sh
sh run_v33_tests.sh
```

Tests use mocked Arduino/PWM/ADC and **do not establish real STM32 binary compatibility**; the target build is checked separately (see `TEST_REPORT.txt`). The `tests/` folder is not part of the Arduino sketch folder; open **only `LFR_PRO_V3/LFR_PRO_V3.ino`** for firmware.

## Recommended real-robot acceptance tests

- Motor direction + PB3 E-stop with wheels raised; hold E-stop and verify no drive output.
- Channel-by-channel 14-sensor white/black sweep; test `LFR_SENSOR_DUMMY_READ=0/1` and MUX settling against alternating surfaces.
- SENSOR LIVE footer shows the scan time: expect roughly 150 US with `LFR_FAST_SENSOR=1`. Reflash with `LFR_FAST_SENSOR=0` once and confirm the raw values match on white and black.
- White/black calibration persistence across power cycle, plus unplug/replug OLED behavior.
- 5+ slow runs each: single line, curved S, 90-degree left/right, disconnected 90-degree turns, zigzag, T/crossing, dashed line gap, full-black start/finish/crossing.
- In each of FAST and TECHNICAL, increase basePWM *gradually*, record completed laps, off-track events and reset causes. Prefer repeatability; do not chase peak speed with poor completion.
- If scripted navigation is used, verify every junction is counted exactly once. For unknown final-round tracks use generalized navigation, not an unverified fixed route script.

**Recommended workflow:** backup -> bench verify -> SAFE 10 controlled laps -> TECHNICAL tune -> FAST tune only after high reliability. Confirm with competition organizer whether the ready-made sensor/driver boards and STM32 controller satisfy the rules.
