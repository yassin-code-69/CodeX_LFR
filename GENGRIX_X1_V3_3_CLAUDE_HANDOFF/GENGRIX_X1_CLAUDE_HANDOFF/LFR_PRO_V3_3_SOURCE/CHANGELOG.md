# LFR PRO V3.3 - Speed Foundation (on top of V3.2)

Goal: groundwork for higher speed and accuracy (KakaRobot-style performance) without changing race behaviour yet. Steering, PID, junction and turn logic are unchanged.

- `LFR_Sensor.cpp/.h`: real direct ADC1 path for `LFR_FAST_SENSOR=1`. ADC1 is configured and calibrated once (12 MHz clock, software trigger, 41.5-cycle sampling); each channel is one register-level conversion. STM32duino `analogRead()` re-initialised, calibrated and de-initialised ADC1 on every call, which also silently discarded the old sample-time write. A conversion that never completes latches `adcFault()`. `lastScanUs()` reports the measured scan time. `LFR_FAST_SENSOR=0` keeps the original `analogRead()` path.
- `LFR_Config.h`: `LFR_ADC_CHANNEL` (PA2 = IN2) and `LFR_ADC_SAMPLE_CYCLES=4` (41.5 cycles, was an ineffective 13.5). SAFE and BALANCED now set their own `junctionAdvanceMs` (it previously leaked from FAST/SPRINT/TECHNICAL).
- `LFR_Storage.cpp`: one flash page erase per SAVE SETTINGS. On the real core `EEPROM.put()` erased and reprogrammed the 1 KB page once per changed byte; the tested host path was different from the hardware path. Both slots still share that one page (documented).
- `LFR_PRO_V3.ino`: AUTO CAL with MIRROR=REVERSED stored values at mirrored indices (fixed). Race result keeps seconds past 99 s and shows worst control-step work time (`LOOP xxxUS`); serial adds `LOOP_MAX_US`, `DT_MAX_US`, `SCAN_US`. SENSOR LIVE footer shows scan time. ADC fault blocks arming and stops a race. Boot message names the sensor path.
- Tests: `tests/run_v33_tests.sh` (direct ADC/MUX register model incl. dummy-read variant, AUTO CAL mirror, single flash write per save, profile junction time, time format, race-loop measurements). Stubs gained `mock_stm32f1.h`, an `analogRead` counter, an ADC hook and a flash-flush counter.
- Real target build (arduino-cli 1.5.1, STMicroelectronics:stm32 3.0.0, BLUEPILL_F103C8, `--warnings all`): 0 warnings; Flash 46,712 B (71%), RAM 4,024 B. V3.2 was 48,952 B / 4,012 B.

**Not done in V3.3:** no hardware flash or run. Fast turns, braking, 1 kHz loop and plan/index mode are the next phases.

# LFR PRO V3.2 - Code Review Change Log

**Source:** unpushed `LFR_PRO_V3.rar`, 14 source files, separated from the GitHub version. Original user archive remains untouched.

## Changed source files

- `LFR_PRO_V3/LFR_Config.h`: bench-safe first boot; optional observer/deadband/reverse/dummy ADC flags; conservative 5us MUX settling; configurable marker and start-pad timeouts; corrected unsupported precision/performance claims.
- `LFR_PRO_V3/LFR_Control.cpp` and `.h`: complete state reset; start pad phase; gap-to-junction/marker reacquisition; sustained finish candidate checks; script route guard; safe black-pad timeout; optional reverse during FOLLOW; more conservative gap command.
- `LFR_PRO_V3/LFR_Sensor.cpp`: fix connected side-fan misclassification; optional dummy ADC read; clarify unverified ADC claims.
- `LFR_PRO_V3/LFR_PID.cpp`: bounded time-adaptive lowpass derivative as default, optional alpha-beta experiment retained.
- `LFR_PRO_V3/LFR_Motor.cpp` and `.h`: measured-dt PWM ramps; cap post-deadband physical PWM; preserve zero-crossing direction reversal.
- `LFR_PRO_V3/LFR_PRO_V3.ino`: race clock after OLED flush; start-pad state passed to controller; measured dt supplied to motor ramp.

## Untouched source modules

`LFR_PID.h`, `LFR_Sensor.h`, `LFR_Storage.cpp`, `LFR_Storage.h`, `LFR_OLED.cpp`, `LFR_OLED.h` remained unchanged. Original EEPROM Settings structure and pin map unchanged. The host test stub definitions are provided separately.

## Deliberately not "automatically fixed"

Unique crossing-vs-finish semantics; ambiguously connected T-junctions; real STM32 timing and flash capacity; actual mechanical tire grip and braking; EEPROM power-loss electrical behavior. These require measurements or contest course details and are listed in the README.

**No GitHub pushes or physical flash operations were performed.**
