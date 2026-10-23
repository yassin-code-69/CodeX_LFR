# LFR PRO V3.2 - Reliability-Corrected Full Source

**Base:** the user's unpushed `LFR_PRO_V3.rar` (14 source files). This is a **conservative software-fix release**, not a hardware-certified or guaranteed winning race firmware. No GitHub repository was changed.

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
3. Compile/Verify and **check the reported Flash and RAM** are within your actual Blue Pill limits (nominal 64KB/20KB). This archive passed PC-host mock tests only; the STM32 toolchain was not available in the audit environment.
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
- Existing UI, storage structure, menu, profiles and core hardware pin mapping preserved.

## Compile-time switch defaults (`LFR_Config.h`)

| Setting | Value | Notes |
|---|---:|---|
| `LFR_ALLOW_RACE_START` | `0` | Set to 1 **after** bench checks; then reflash |
| `LFR_FAST_SENSOR` | `1` | Fast GPIO MUX; compare against fallback mode on real hardware |
| `LFR_SENSOR_DUMMY_READ` | `0` | Optional discard-first-ADC for channel crosstalk comparisons |
| `LFR_PID_USE_ALPHA_BETA` | `0` | Experimental estimator option remains, default is stable lowpass D |
| `LFR_MOTOR_DEADBAND` | `0` | Only enable from measured stiction threshold; final physical duty is capped |
| `LFR_ALLOW_FOLLOW_REVERSE` | `0` | Avoid forced reverse in FOLLOW before grip tests |
| `CTRL_PERIOD_US` | `3000` | Nominal ~333Hz, contingent on sensor scan budget |
| `LFR_FINISH_CONFIRM_MS` | `24` | Confirmed full marker dwell; may miss very thin markers |
| `LFR_MARKER_TIMEOUT_MS` | `900` | Safety stop if black area has no usable line for too long |

## Remaining limitations - READ BEFORE COMPETITION

1. **A full-black finish pad and a large full-black intersection can look identical** to a 14-channel reflectance array; software cannot guarantee a correct finish without course semantics. **Leave FINISH OFF until organizer rules and sensor replay tests are available.**
2. An optically **connected T junction** can look like a broad 90-degree corner, so conservative classification can miss some true junctions. Build sensor replay sequences for the actual course and confirm expected route behavior.
3. No wheel encoders, IMU or camera. Absolute traveled distance, real RPM, and reliable shortest-route mapping are unavailable.
4. The 510-530g mass is below 1 kg, but **does not justify exact PWM/gain/pivot values**. Tire material, gearing, chassis geometry, sensor height, battery voltage and flooring dominate tuning.
5. Physical PB3 E-stop, startup/remap, ADC channel settling and STM32 Flash/EEPROM emulation **must** be validated on your board. Flash save power loss protection was not proven in situ.
6. A software E-stop cannot replace a physical motor-power disconnect. Do not run raised-wheel tests with the wheels touching objects or fingers.
7. This ZIP was **not** uploaded to or run on the robot. Do not assume national competition readiness from host test results.

## Host regression tests

On Linux/macOS with `g++`, from the `tests` folder run:

```sh
sh run_tests.sh
sh run_v32_regressions.sh
```

Tests use mocked Arduino/PWM/ADC and **do not establish real STM32 binary compatibility**. The `tests/` folder is not part of the Arduino sketch folder; open **only `LFR_PRO_V3/LFR_PRO_V3.ino`** for firmware.

## Recommended real-robot acceptance tests

- Motor direction + PB3 E-stop with wheels raised; hold E-stop and verify no drive output.
- Channel-by-channel 14-sensor white/black sweep; test `LFR_SENSOR_DUMMY_READ=0/1` and MUX settling against alternating surfaces.
- White/black calibration persistence across power cycle, plus unplug/replug OLED behavior.
- 5+ slow runs each: single line, curved S, 90-degree left/right, disconnected 90-degree turns, zigzag, T/crossing, dashed line gap, full-black start/finish/crossing.
- In each of FAST and TECHNICAL, increase basePWM *gradually*, record completed laps, off-track events and reset causes. Prefer repeatability; do not chase peak speed with poor completion.
- If scripted navigation is used, verify every junction is counted exactly once. For unknown final-round tracks use generalized navigation, not an unverified fixed route script.

**Recommended workflow:** backup -> bench verify -> SAFE 10 controlled laps -> TECHNICAL tune -> FAST tune only after high reliability. Confirm with competition organizer whether the ready-made sensor/driver boards and STM32 controller satisfy the rules.
