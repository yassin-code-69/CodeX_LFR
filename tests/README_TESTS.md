# Host Test Suite

Tests are on a simulated Arduino API, not the real STM32F103C8T6 processor.

- `run_tests.sh`: original host regression suites (PID, motor, sensor, EEPROM, migration, OLED and UI).
- `run_v32_regressions.sh`: tests for turbo restarts, GAP->junction/marker reacquisition, corner fan, start pad/finish, script route finish guard, black-marker timeout, Gap STOP, motor PWM cap, dt-dependent ramp and PID transient.
- `test_v32_properties.cpp`: 2200 deterministic randomized controller traces, PWM bounds and opposing BTS7960 pins never both positive.
- `run_v33_tests.sh`: V3.3 foundation. `test_fast_adc.cpp` runs the direct STM32F1 sensor path (BSRR MUX select + ADC1 registers, also with `LFR_SENSOR_DUMMY_READ=1`) against `stubs/mock_stm32f1.h`; `test_v33_foundation.cpp` checks AUTO CAL with MIRROR, one flash write per save, profile junction time, race time past 99 s and the race-loop timing measurements.
- `run_v352_tests.sh`: V3.5.2 safety audit. `test_v352_safety_audit.cpp`: SCRIPT may not turn into an unseen branch, FAST CATCH line loss keeps the step pending, CATCH motor outputs through the real slew show no reverse PWM in any profile.
- `run_v35_tests.sh`: V3.5. `test_v35_performance.cpp` (FAST turn phases vs CLASSIC, guards, SCRIPT commit via CATCH, 2 x 100,000-tick seeded stress) and `test_v35_ui.cpp` (TURN MODE menu is RAM only, CSV telemetry drops rows instead of blocking).
- `run_v34_tests.sh`: V3.4 Phase A. `test_v34_navigation.cpp` (T/+ crossings, side branch vs corner, pads, finish robustness, SCRIPT transaction, gap budget, WHITE-mode black start pad; production Sensor+Controller) and `test_v34_safety.cpp` (real sketch with the direct ADC register model: dying/dead ADC during AUTO SWEEP and scans, stuck calibration, blank flash boot, WHITE-mode arming).
- `stubs/`: simulated ADC, PWM, EEPROM and I2C API. `mock_stm32f1.h` models the few STM32F1 registers the direct sensor path uses.

The archive includes the tests as source; binaries generated during development are not included. To build on Linux/macOS, `cd tests && sh run_tests.sh && sh run_v32_regressions.sh && sh run_v33_tests.sh && sh run_v34_tests.sh && sh run_v35_tests.sh && sh run_v352_tests.sh`.

On Windows, run via MSYS2, Git Bash (with g++) or WSL. This is independent of Arduino IDE's actual Verify operation, which remains mandatory.
