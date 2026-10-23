# Host Test Suite

Tests are on a simulated Arduino API, not the real STM32F103C8T6 processor.

- `run_tests.sh`: original host regression suites (PID, motor, sensor, EEPROM, migration, OLED and UI).
- `run_v32_regressions.sh`: tests for turbo restarts, GAP->junction/marker reacquisition, corner fan, start pad/finish, script route finish guard, black-marker timeout, Gap STOP, motor PWM cap, dt-dependent ramp and PID transient.
- `test_v32_properties.cpp`: 2200 deterministic randomized controller traces, PWM bounds and opposing BTS7960 pins never both positive.
- `stubs/`: simulated ADC, PWM, EEPROM and I2C API.

The archive includes the tests as source; binaries generated during development are not included. To build on Linux/macOS, `cd tests && sh run_tests.sh && sh run_v32_regressions.sh`.

On Windows, run via MSYS2, Git Bash (with g++) or WSL. This is independent of Arduino IDE's actual Verify operation, which remains mandatory.
