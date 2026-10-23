# GENGRIX-X1 full project audit (2026-10-23, V3.5.1)

Scope: all 14 firmware files in `LFR_PRO_V3/`, all host tests and mocks, docs, git state, checksum manifest, real STM32 target build. Method: line-by-line review, reproduction of every suspected defect on host before fixing, fault injection, UBSan (trap mode), arduino-cli builds. **No physical bench or track test was possible** - every runtime claim below is host-simulated.

## 1. Workspace and repository

| Check | Result |
|---|---|
| Active firmware | `LFR_PRO_V3/` only (see `WORKSPACE.md`) |
| Duplicate source | `GENGRIX_X1_V3_3_CLAUDE_HANDOFF/.../LFR_PRO_V3_3_SOURCE/` = V3.3 copy, reference only (committed in `ad1a7fb`) |
| `main` | `2bb762d` (V3.3), untouched |
| `dev/v3.4-phase-a` | `567b16d` V3.4.1 (local, 2 commits ahead of origin) |
| `dev/v3.5-performance` | V3.5.1 (local only) |
| `FILES_SHA256.txt` | regenerated, all entries verify |
| Stale docs | `TECHNICAL_README.md` / `TEST_REPORT.txt` are V3.3-era; marked historical, wrong finish constant corrected |
| Cosmetic | `tests/previews/*.pgm` show the V3.3 screens (regenerated only in test copies) |

## 2. Hardware lock

`git diff main -- LFR_PRO_V3` contains no change to any pin, the four buttons (PB13/PB12/PB5/PB3), MUX table, 10 kHz PWM, Settings struct or EEPROM record layout. Details: `PIN_HARDWARE_LOCK.md`.

## 3. Defects found in this audit (all reproduced first, all fixed, regression `testAuditFixes`)

| ID | Severity | Defect | Fix |
|---|---|---|---|
| A | P0 (SCRIPT) | Leaving a black start pad at an angle produced a both-edge junction pattern and proposed a route step | broad frames stay in the pad phase; pad exit latches the junction detector |
| B | P0 (SCRIPT) | A STRAIGHT step only committed with the line under the centre sensors; on a curve it never committed and **all later junctions were ignored** | commit on any narrow line past the junction |
| C | P0 (SCRIPT) | A + crossed at an angle (one arm, then the other) offered only the first arm -> SCRIPT RIGHT stopped with FAULT | collect every arm seen during the candidate |
| D | P1 | After a crossing, "straight exists" needed the centre sensors; an off-centre continuing line made a + look like a T (STRAIGHT step -> FAULT) | straight exists if any line continues |
| E | P3 | `TECHNICAL_README.md` listed `LFR_FINISH_CONFIRM_MS = 24` (actual 250) | corrected |

A, B, C, D fail on V3.4 `867289e` and pass now. A existed since V3.2 (start pad); B-D came with the V3.4 junction work.

## 4. Earlier Phase A fixes re-verified

F01 T/+ and side branch, F02 finish robustness, F03 WHITE-mode black start pad, F04 route transaction, F05 auto-sweep ADC abort, F06 ADC init fault latch, F07 skip-identical save / blank-flash lock, F08 gap budget: all regression tests still pass on V3.4.1 and V3.5.1. The audit repro tests from `V33_AUDIT_REPRO_TESTS.zip` fail exactly at the old-behaviour assertions.

## 5. Module assessment

| Module | Assessment | Open items |
|---|---|---|
| `LFR_Sensor` | Q16 normalisation bounded (range >= 250 guarantees no 32-bit overflow), blob loop terminates, centroid sums < 2^31, ADC waits bounded, init/conversion faults latched, fast-fail after fault | Real ADC accuracy, crosstalk, scan time, PA2 voltage unmeasured; analogRead fallback cannot detect faults; 1-D position (U-shaped bar geometry unknown) |
| `LFR_Control` | Every state time-bounded (pivot 800-1450 ms, gap 300, marker 900, start pad 1500, brake 60, catch 15); route transaction; FAST phases isolated from CLASSIC | Timing constants are speed dependent and untuned; optical ambiguities remain (sec. 7) |
| `LFR_PID` | dt-guarded, bounded derivative, anti-windup, output clamped | Gains untuned on this chassis |
| `LFR_Motor` | Clamp to maxPWM, reversal through zero, H-bridge pins never both driven (property-tested), dt-scaled slew | Real braking / stiction unmeasured |
| `LFR_Storage` | CRC + dual slot + legacy migration, one erase per changed save, verify after write | Both slots in one flash page: power cut during SAVE can lose both (boots SAFE, refuses to arm) |
| `LFR_OLED` | Unchanged; host snapshots pass | Missing OLED stops the robot at boot (by design) |
| `LFR_PRO_V3.ino` | BACK polled before every motor update in Race/MotorTest/Countdown; ADC fault gates arming, auto-sweep and scans; no OLED or flash writes in the race loop; telemetry drops instead of blocking | No hardware watchdog; `soundChime` blocks <= 250 ms (motors already stopped); simultaneous button presses report only one |

## 6. Evidence

- Host: 62 PASS lines, 0 failures (`run_tests.sh`, `run_v32_regressions.sh`, `run_v33_tests.sh`, `run_v34_tests.sh`, `run_v35_tests.sh`), also 62/62 under UBSan trap mode. Includes 2200 + 2 x 100,000 seeded random controller ticks with PWM-bound and interlock invariants. ASan unavailable on MinGW.
- STM32F103C8 (core 3.0.0, `--warnings all`, 0 warnings), V3.5.1: default 48,732 B / 4,088 B RAM; `FAST_SENSOR=0` 51,352 B; `RACE_START=1` 49,712 B; `RACE_START=1 TELEMETRY=1` 50,220 B. V3.4.1 default 47,752 B.
- Logs: `test_logs/v341/`, `test_logs/v351/`.

## 7. Remaining risks (not fixable without measurement)

See `REMAINING_RISKS.md`. Highest: no physical validation yet; no independent hardware watchdog; optical ambiguity between crossing / pad / finish / corner decided by untuned timing; storage not power-cut atomic; FAST turn constants are guesses; four buttons vs a reported two-switch rule needs the organiser.

## 8. Before the track

1. Flash the RACE_LOCKED build, wheels off the ground, follow `TOMORROW_UNIVERSITY_TEST_PLAN.md` section 1.
2. Measure and record `SCAN_US`, `LOOP xxxUS`, the crossing time of a 3 cm line (telemetry STATE=5 duration) and the 10 cm gap crossing time; set `LFR_CROSSING_MAX_MS` and `LFR_GAP_RECOVER_MS` from them.
3. CLASSIC before FAST; SAFE before BALANCED; FINISH OFF until the finish rule is known.
