# LFR PRO V3.5 - Performance candidate (branch `dev/v3.5-performance`, from V3.4 `867289e`)

Everything new is OFF / CLASSIC by default; hardware, pins, four buttons, PWM and EEPROM format unchanged. Not physically tested.

- **TURN MODE (menu item 16): CLASSIC / FAST, RAM only** (CLASSIC after every power-up, never saved, so an experimental mode cannot auto-arm).
- **FAST turn phases** (`LFR_Control.cpp`): corner detection -> `BRAKE` (zero command, 25 ms per 100 PWM of recent commanded forward PWM, max 60 ms; a centred line cancels it) -> `PIVOT` -> early `CATCH` when the new line enters +-2600 from the turning side (opposite pulse at half turn PWM for 15 ms) -> `FOLLOW` with a 150 ms speed ramp from curve to base PWM. SCRIPT steps commit at CATCH exit. CLASSIC path is unchanged and is what every pre-V3.5 test exercises.
- Speed proxy is the smoothed commanded PWM, explicitly not a measured speed (no encoders).
- **Telemetry** (`LFR_RACE_TELEMETRY 1`): CSV at ~50 Hz on Serial1 (state, profile, turn mode, sensor flags, position, confidence, junctions, route, PWM, scan and loop time); a row is dropped, never waited for, when the UART buffer lacks room. Arm screen tag shows `FAST` when FAST turn mode is selected.
- Tests: `tests/run_v35_tests.sh` (FAST phases and guards, CLASSIC unchanged, SCRIPT commit through CATCH, 2 x 100,000-tick seeded stress with PWM-bound/interlock/phase-duration invariants, TURN MODE menu, telemetry drop-not-block). All 61 host PASS lines also pass under UBSan trap mode.
- STM32 (0 warnings): default 48,672 B flash / 4,088 B RAM; `FAST_SENSOR=0` 51,292 / 4,092; `RACE_START=1 TELEMETRY=1` 50,160 / 4,104.
- Docs: `docs/ARCHITECTURE.md`, `CONFIG_PARAMETERS.md`, `PIN_HARDWARE_LOCK.md`, `TEST_RESULTS.md`, `REMAINING_RISKS.md`, `TOMORROW_UNIVERSITY_TEST_PLAN.md`; `WORKSPACE.md` names the active source tree.
# LFR PRO V3.4.1 - audit fixes (branch `dev/v3.4-phase-a`)

Found by a full code audit of V3.4/V3.5 and reproduced on host before fixing:

| ID | Root cause | Fix | Test |
|---|---|---|---|
| A | Leaving a black start pad at an angle shows a both-edge "junction" pattern; it proposed a SCRIPT step | broad frames stay in the start-pad phase; leaving the pad latches the junction detector like a marker exit | `testAuditFixes` A |
| B | A SCRIPT STRAIGHT step committed only on frames with the line under the centre sensors; off-centre (curve) it never committed, so every later junction was ignored | commit on any narrow line past the junction | B |
| C | A + crossed at an angle shows one arm, then the other; only the first fan side was offered, so SCRIPT RIGHT stopped with FAULT | remember every arm seen during the side-branch candidate | C |
| D | After a crossing, "straight exists" required the centre sensors; an off-centre continuing line made a + look like a T | straight exists if any line continues | D |

The new test fails on V3.4 (`867289e`) and passes now. Docs: stale `LFR_FINISH_CONFIRM_MS 24` in `TECHNICAL_README.md` corrected; historical notes added to `TECHNICAL_README.md` and `TEST_REPORT.txt`.

# LFR PRO V3.4 - Phase A reliability and safety (branch `dev/v3.4-phase-a`)

Hardware, pins, the four buttons (UP PB13, DOWN PB12, ENTER PB5, BACK/E-STOP PB3), PWM frequency, menus and the EEPROM record format are unchanged. Every fix below has a host regression test; none is physically validated yet.

| ID | Root cause (V3.3) | Change | Regression test | Remaining limitation |
|---|---|---|---|---|
| F01 T/+ | A crossing line darkens all 14 sensors, so it was `fullMarker` (pad) and never a junction; a T then ended GAP -> FAULT, a + never consumed its SCRIPT step | Marker that clears within `LFR_CROSSING_MAX_MS` (200 ms) after an armed entry is an intersection: both arms exist, straight exists only if the line continues; normal junction decision (`takeJunction`) | `test_v34_navigation`: T (STRAIGHT -> left, SCRIPT R), +, SCRIPT S at a T stops safely, sensor-level T through real `Sensor::read` | A narrow pad crossed faster than 200 ms, or a crossing crossed slower than 200 ms, is misread. Tune the constant on the real track. |
| F01 side branch | A connected edge-to-centre blob is also a 90-degree corner, so V3.2 never counted it | Branch is a junction only if it follows a clean approach and the narrow centred line resumes within 120 ms; if the line is lost instead it is a corner and the robot pivots to the fan side | side branch STRAIGHT/SCRIPT L, wrong-side SCRIPT stops, corner pivots right, fan without clean approach ignored | Optically identical patterns remain ambiguous; physical replay needed |
| F02 finish | 24 ms confirmation made a broad crossing a finish; one non-clean frame zeroed the clean-approach counter | Finish confirm 250 ms (> crossing max, static_assert); non-clean frames shorter than 40 ms keep the approach history | crossing with FINISH ON is a junction, not FINISHED; one glitch frame still finishes | Finish and a black area wider than the bar are indistinguishable optically; FINISH stays OFF by default |
| F03 white mode | `fullMarker` follows the selected line colour; a black pad in WHITE mode is "no line", arming refused | `SensorFrame::fullBlack` (unfiltered, polarity independent); arming and start-pad phase accept it, pad phase ends when the line is seen, 1500 ms bound | sensor + controller test; sketch test arms on a black pad in WHITE mode, drives off, BACK stops | Pad direction must point at the line |
| F04 route | `routeUsed_++` inside `choose()` before the manoeuvre | propose -> execute -> commit: turns commit on pivot exit (line centred), STRAIGHT after 3 clean frames; no new junction while pending; FAULT leaves the step uncommitted; finish requires no pending step | `test_v3` script test, failed pivot keeps routeUsed 0, repeated junction frames counted once | Route is still junction-count based, not checkpoint aware (F13) |
| F05 auto-sweep | ADC fault was never checked while motors swept | one scan with motors stopped first; scan before every drive step, fault -> motors stop, notice; manual scan discards faulted captures | ADC dying mid-sweep stops after the faulty scan (was 900 ms); latched fault never drives, scans store nothing | analogRead fallback (`LFR_FAST_SENSOR 0`) cannot detect ADC faults |
| F06 ADC init | RSTCAL/CAL/ADON completion not validated | fault latched if reset/calibration bits stay set or ADON/clock missing; latched fault makes conversions return at once | stuck calibration -> `adcFault`, next scan does no conversions | Real-silicon fault rate unknown |
| F07 storage | both slots in one 1 KB page (documented in V3.3) | identical saves skip the erase; corrupt/blank flash boots SAFE defaults with no calibration so arming is refused | unchanged save writes nothing; blank flash -> defaults, arming refused | A power cut during SAVE can still lose both slots (no two-page journal yet) |
| F08 gap | fixed 180 ms | `LFR_GAP_RECOVER_MS` 300 ms (STOP 55 ms), bounded | 250 ms gap recovers; >300 ms stops | 10 cm needs >= 0.33 m/s at curve PWM; measure |
| F09 switches | rule says max two switches | **no change**: all four buttons kept; flagged for the organiser | - | competition compliance unresolved |
| F12 patch | `SOURCE_CHANGES.patch` is V3.2 only | labelled historical in `WORKSPACE.md` | - | - |

Also: OLED shows `ADC ERR` in the menu and SENSOR LIVE header when the ADC fault is latched.

Real STM32 build (arduino-cli 1.5.1, STM32 core 3.0.0, BLUEPILL_F103C8, `--warnings all`, 0 warnings): default 47,696 B flash / 4,052 B RAM; `LFR_FAST_SENSOR=0` 50,316 B / 4,056 B; `DUMMY_READ=1 + RACE_START=1` 48,656 B / 4,068 B. V3.3 default was 46,712 B / 4,024 B.

Intentionally changed expectations in older tests: gap budget 180 -> 300 ms (`test_v3`), finish confirm 24 -> 250 ms (`test_v32_regressions`), unchanged saves no longer write (`test_v33_foundation`), SCRIPT step committed after reacquire (`test_v3`).

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
