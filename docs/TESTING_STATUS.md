# Testing status (baseline V3.5.2, commit `905389c`)

**Physical bench test: NOT PERFORMED. Track test: NOT PERFORMED.** Everything below is host simulation or a real STM32 *compile*; none of it shows how the robot behaves.

## Host regression suites (`tests/`, simulated Arduino/STM32)

| Script | Covers | Result |
|---|---|---|
| `run_tests.sh` | PID, motor, sensor, storage + legacy migration, OLED, UI smoke, mode editor | PASS |
| `run_v32_regressions.sh` | gap/junction/marker/start-pad/finish policies, motor cap and ramp, 2,200 random controller traces | PASS |
| `run_v33_tests.sh` | direct ADC1/MUX register model, AUTO CAL mirror, flash write count, timing diagnostics | PASS |
| `run_v34_tests.sh` | T/+ crossings, side branch vs corner, pads, finish, SCRIPT transaction, gap budget, WHITE-mode start pad, ADC fault injection, audit fixes | PASS |
| `run_v35_tests.sh` | FAST turn phases vs CLASSIC, 2 x 100,000-tick seeded stress, TURN MODE menu, telemetry | PASS |
| `run_v352_tests.sh` | SCRIPT never turns into an unseen branch, CATCH line loss keeps the step, CATCH motor outputs (no reverse PWM) | PASS |

Total: **69 PASS lines, 0 failures**; also 69/69 with UndefinedBehaviorSanitizer in trap mode. AddressSanitizer is not available with MinGW.

## STM32F103C8 target builds (arduino-cli 1.5.1, STM32 core 3.0.0, BluePill F103C8, `--warnings all`)

| Build | Flash (of 65,536 B) | RAM (of 20,480 B) | Warnings |
|---|---|---|---|
| RACE_LOCKED (default) | 48,780 B (74%) | 4,088 B | 0 |
| RACE_UNLOCKED (`-DLFR_ALLOW_RACE_START=1`) | 49,764 B | 4,096 B | 0 |
| `-DLFR_FAST_SENSOR=0` | 51,400 B | 4,092 B | 0 |
| `-DLFR_ALLOW_RACE_START=1 -DLFR_RACE_TELEMETRY=1` | 50,272 B | 4,104 B | 0 |

RAM figures are static allocation only; stack peak is unmeasured.

## Where the evidence lives

Full logs, verification test sources, binaries and SHA256 manifests are kept outside the repository in `CodeX_LFR_RELEASES/V3.5.2_905389c/` (and earlier packages). The commit history of `dev/v3.5.2-safety-audit` also contains the per-version `test_logs/`.

## Still to be validated on the robot

Everything in the feature list: ADC values and scan time, E-STOP latency, motor directions and stop behaviour, calibration persistence, line following and every navigation element, FAST turn behaviour, telemetry. See `TRACK_TEST_PLAN.md` and `KNOWN_LIMITATIONS.md`.
