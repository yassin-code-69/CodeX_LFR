# Test results (2026-10-23)

Environment: Windows 11, MinGW g++ (Git Bash), arduino-cli 1.5.1 bundled with Arduino IDE, STMicroelectronics:stm32 3.0.0, board GenF1 `pnum=BLUEPILL_F103C8`. Logs: `test_logs/v34/`, `test_logs/v35/`.

## Scorecard

| Item | Completed | Host verified | STM32 build verified | Physical test |
|---|---|---|---|---|
| V3.4 ADC init/scan/auto-sweep fault safety (F05/F06) | yes | PASS | PASS | PENDING |
| V3.4 T/+ crossing junctions (F01) | yes | PASS (incl. real Sensor::read T) | PASS | PENDING |
| V3.4 side branch vs 90-degree corner (F01) | yes | PASS | PASS | PENDING |
| V3.4 finish robustness (F02) | yes | PASS | PASS | PENDING |
| V3.4 WHITE mode black start pad (F03) | yes | PASS | PASS | PENDING |
| V3.4 SCRIPT route transaction (F04) | yes | PASS | PASS | PENDING |
| V3.4 storage: skip identical save, blank flash locked (F07) | partial (no 2-page journal) | PASS | PASS | PENDING |
| V3.4 gap budget 300 ms (F08) | yes | PASS | PASS | PENDING |
| V3.5 FAST turn phases (BRAKE/CATCH/exit ramp), CLASSIC default | yes | PASS | PASS | PENDING |
| V3.5 TURN MODE menu (RAM only) | yes | PASS | PASS | PENDING |
| V3.5 CSV telemetry (drop-not-block) | yes | PASS | PASS | PENDING |
| Switch-count rule (F09) | blocked: organiser decision | - | - | - |
| Hardware watchdog, 2-page flash journal, per-sensor XY model, 1 kHz loop | not started | - | - | - |

## Host suites (simulated hardware)

| Command (in `tests/`, on a copy) | V3.4 | V3.5 |
|---|---|---|
| `sh run_tests.sh` | PASS | PASS |
| `sh run_v32_regressions.sh` (incl. 2200 random traces) | PASS | PASS |
| `sh run_v33_tests.sh` | PASS | PASS |
| `sh run_v34_tests.sh` (9 navigation + 5 safety/fault-injection) | PASS | PASS |
| `sh run_v35_tests.sh` (FAST/CLASSIC, 2 x 100,000-tick seeded stress, UI/telemetry) | - | PASS |
| PASS lines total | 53 | 61 |

V3.5.2 safety-audit branch (not a release): 69 PASS lines, 0 failures, also under UBSan; `run_v352_tests.sh` added (`test_logs/v352/`). STM32: LOCKED 48,780 B, UNLOCKED 49,764 B, FAST_SENSOR=0 51,400 B, TELEMETRY 50,272 B, 0 warnings.

After the full audit (V3.4.1 / V3.5.1, see `AUDIT_REPORT.md`): 54 / 62 PASS lines, 0 failures; V3.5.1 also 62/62 under UBSan trap mode (`test_logs/v351/`).

Audit repro tests (`V33_AUDIT_REPRO_TESTS.zip`) against V3.4: they encode V3.3 behaviour and now fail exactly at the fixed points (route consumed before turn, 900 ms sweep, init fault not latched).

Intentionally updated expectations in older tests: gap 180 -> 300 ms, finish confirm 24 -> 250 ms, unchanged saves do not write, SCRIPT step commits after reacquire.

UndefinedBehaviorSanitizer in trap mode (`CXX="g++ -fsanitize=undefined -fsanitize-trap=undefined"`): all five suites PASS (61), no traps (`test_logs/v35/v35_host_tests_ubsan.log`). AddressSanitizer: NOT AVAILABLE (MinGW has no libasan) - UNVERIFIED.

## STM32F103C8 target builds (`--warnings all`, 0 warnings in every build)

| Build | Flash (of 65,536) | RAM (of 20,480) |
|---|---|---|
| V3.3 `main` default | 46,712 B | 4,024 B |
| V3.4 default | 47,696 B | 4,052 B |
| V3.4 `LFR_FAST_SENSOR=0` | 50,316 B | 4,056 B |
| V3.4 `DUMMY_READ=1 RACE_START=1` | 48,656 B | 4,068 B |
| V3.5 default | 48,672 B (74%) | 4,088 B |
| V3.5 `LFR_FAST_SENSOR=0` | 51,292 B (78%) | 4,092 B |
| V3.5 `RACE_START=1 TELEMETRY=1` | 50,160 B (76%) | 4,104 B |
| V3.5 `DUMMY_READ=1` | 48,680 B | 4,088 B |
| V3.4.1 default (audit fixes) | 47,752 B | 4,052 B |
| V3.5.1 default (audit fixes) | 48,732 B (74%) | 4,088 B |
| V3.5.1 `LFR_FAST_SENSOR=0` | 51,352 B | 4,092 B |
| V3.5.1 `RACE_START=1` | 49,712 B | 4,096 B |
| V3.5.1 `RACE_START=1 TELEMETRY=1` | 50,220 B | 4,104 B |

RAM figures are static only; stack peak is unmeasured. Firmware defaults to race locked (`LFR_ALLOW_RACE_START 0`).

## Physical
Bench test: NOT PERFORMED. Track test: NOT PERFORMED. No speed, lap time or turn performance is claimed.
