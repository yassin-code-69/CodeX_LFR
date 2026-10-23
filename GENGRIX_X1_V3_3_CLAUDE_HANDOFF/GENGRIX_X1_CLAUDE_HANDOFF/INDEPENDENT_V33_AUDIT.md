# GENGRIX-X1 / LFR PRO V3.3 -- Independent Full Firmware Audit

**Input archive:** `CodeX_LFR.rar` (RAR5); inspected in a safe, isolated working copy.  
**Date:** 2026-10-23  
**Scope:** All 14 production firmware files, all supplied host tests and mock environment, documentation, checksum manifest, supplied Git history and patch.  
**Hardware context:** STM32F103C8T6, 14-channel analog IR via PA2 MUX, two BTS7960 drivers, SSD1306, four navigation buttons, 510-530 g robot including battery. Actual board, track, motor RPM under load, power consumption and sensor voltage have **not** been measured.

## Verdict

**V3.3 is a credible, largely well-isolated foundation update, but NOT cleared for competition or high-speed turning.** The previously known junction/marker/route issues remain. The new direct ADC path is testable and promising but requires physical cross-checks. Two further ADC safety gaps were found.

Classification:
- **REPRODUCED:** Produced on the submitted, unmodified production code under host/mocked hardware using separate independent tests.
- **SOURCE CONFIRMED:** Unambiguous source behavior or supported by the STM32duino core implementation; physical consequence not proven.
- **CONDITIONAL:** Failure depends on track shape, marker layout, or sensor readings.
- **UNVERIFIED:** Needs target compilation evidence and/or physical tests.

## Archive integrity, provenance and comparison

- The entire RAR was extracted with libarchive to an isolated directory; no source code was modified.
- All **30** file hashes in `FILES_SHA256.txt` pass `sha256sum -c`.
- The embedded local Git `main` branch is **clean**, with HEAD `2bb762deccb24541ee4a5a4ae698e4066ef08f6f` (`feat: add V3.3 speed foundation...`). The statement that V3.3 changes are still *uncommitted* no longer describes this archive. This does **not** establish that the commit was pushed. The configured remote returned 404 through the accessible public GitHub interface; push status is not independently confirmed.
- Byte-for-byte comparison against the previously delivered `LFR_PRO_V3_2_CORRECTED_FULL_CODE.zip`: only `LFR_Config.h`, `LFR_Sensor.cpp`, `LFR_Sensor.h`, `LFR_Storage.cpp`, and `LFR_PRO_V3.ino` changed. The nine other production files are identical to V3.2.
- `SOURCE_CHANGES.patch` appears to be the earlier V3.2 corrective patch; it does **not** include the V3.3 buffered EEPROM or direct ADC changes. Do not treat it as the complete V3.3 delta. Use `git diff 90a475e 2bb762d -- LFR_PRO_V3/` or the separately supplied `V32_TO_V33_FIRMWARE.patch`.

## Tests I executed independently

In a separate copy of the extracted archive:

| Executed test | Outcome | Meaning |
| --- | --- | --- |
| `tests/run_tests.sh` | PASS | PID, basic motor/sensor, EEPROM mock, OLED/UI mock and sketch syntax |
| `tests/run_v32_regressions.sh` | PASS | Earlier navigation safety regressions and 2,200 deterministic controller traces |
| `tests/run_v33_tests.sh` | PASS | Direct ADC register **mock**, dummy-read enabled test, calibration mirror, EEPROM mock and UI timing |
| **Total original `PASS` lines** | **39** | The original suites each contain multiple assertions; 39 is not 39 hardware test runs |
| Same 3 scripts with ASan + UBSan | PASS | No sanitizer findings in tested host paths |
| V3.3 foundation sketch compiled and run with `-DSTM32F1xx` and a register-level mock | PASS | Extended host integration coverage, still *not* a microcontroller target build |
| New open-risk / fault injection tests | Behaviors REPRODUCED | See below |
| Arduino CLI STM32 target Verify | **NOT performed here** | Compiler/core not present in this environment |
| Real STM32 / sensor / motor / track test | **NOT performed** | Must happen on the physical device |

The supplied `TEST_REPORT.txt` claims **0 compiler warnings** on `STMicroelectronics:stm32` core **3.0.0** with Blue Pill F103C8, **46,712 B flash (71%)** and **4,024 B RAM** in default V3.3. These numbers are **reported by the project's previous compiler**, not independently reproduced here. Global/static RAM size alone does not establish maximum runtime stack consumption.

### Independent reproducible observations

`reproduction/test_open_risks.cpp` executed against the production sensor/controller/PID:

1. All 14 sensors black: `fullMarker=1`, `junction=0`.
2. Connected left-edge-to-center pattern (8 adjacent sensors): `junction=0` though such a pattern can also represent a connected T/side branch.
3. Scripted route index advances to 1 before the planned turn even begins; after a forced failed pivot, it remains consumed.
4. Under a valid approach sequence and `FINISH=ON`, a sufficiently broad black crossing is classified `FINISHED`.
5. A single atypical centered-approach frame preceding a sustained black pad can instead lead to `FAULT`, even with `FINISH=ON`.
6. In `WHITE` line mode, an all-black start pad is not considered a line/marker (`seen=0`, `fullMarker=0`). If the event starts on an all-black square without a white segment, `raceStart()` rejects it.

`reproduction/test_adc_calibration_abort.cpp`: with the ADC mock intentionally dead before auto-sweep, `adcFault=1`, but auto-sweep runs through its **900 ms** time window before motors are stopped; it does not abort when the ADC fault first appears.

`reproduction/test_adc_init_timeout.cpp` with an audit-only stuck-calibration register model: `RSTCAL` and `CAL` remain set after the bounded init loops, but `adcFault=0`; init completion is not validated.

These independent tests demonstrate source behavior in **mocked conditions**. They do not prove how frequently those cases occur on a real track.

## Production findings -- ordered by required action

### P0 / Race-blocking (depending on event layout)

**F01 -- All-black marker vs connected T/Cross junction ambiguity**  
Source: `LFR_PRO_V3/LFR_Sensor.cpp:178-192`, `LFR_Control.cpp:273-280`, `LFR_Control.cpp:313-335`.  
`fullMarker` takes priority over `junction` for large black patterns. A contiguous left fan can be rejected as a junction to prevent false 90-degree turns. Both decisions are defensible conservative choices, but can miss an actual T/crossing. A required checkpoint path could be bypassed or the robot may enter Marker rather than selecting the scripted turn. **Reproduced.**  
**Action:** Record sensor frames for representative actual T, cross, 90-degree bend and black pad. Build a spatial/temporal junction candidate and exit-confirmation policy. Do not force every contiguous fan to be a junction: that would reintroduce false sharp-turn detection.

**F02 -- False/missed FINISH from identical reflectance geometry**  
Source: `LFR_PRO_V3/LFR_Control.cpp:173-177`, `233-255`, `273-280`.  
A strong centered approach followed by a broad black intersection can stop as a finish if `finishOn=1`. One noisy approach frame can reset the eligibility counter, causing a true black finish pad to end as a `FAULT`. **Both reproduced in sensor-frame simulations.**  
**Action:** Keep FINISH OFF until tested on the actual event layout. Add a configurable minimum-run or checkpoint gate, temporal hysteresis, explicit terminal policy and course-specific replay tests. With only 14 reflectance sensors, visually identical black crossing and finish cannot always be distinguished.

**F03 -- White-line mode may reject a black START square**  
Source: `LFR_PRO_V3/LFR_Sensor.cpp:130-155`, `178-192`; `LFR_PRO_V3/LFR_PRO_V3.ino:635-639`.  
`fullMarker` tracks the *selected line* rather than absolute black. When following white line, all-black gives zero normalized line response. The arm guard then rejects an all-black start zone. **Reproduced** for a homogeneous black pad with no visible white segment.  
**Action:** Define start-pad detection separately from line polarity, based on calibrated raw/black coverage; require safe physical confirmation before arming. Track rules and exact start positioning must be checked.

**F04 -- Script route step commits before turn success**  
Source: `LFR_PRO_V3/LFR_Control.cpp:57-61` and `323-334`.  
`routeUsed_++` happens when the junction is detected; a later failed or ambiguous turn leaves the step consumed. **Reproduced.**  
**Action:** Separate `pendingRouteIndex` from `committedRouteIndex`; commit only after exit and line reacquisition, including handling for STRAIGHT. Add duplicate-junction and pivot-failure tests.

### P1 / Safety and correctness

**F05 -- AUTO SWEEP ignores latched ADC fault while motors move**  
Source: `LFR_PRO_V3/LFR_PRO_V3.ino:350-369` versus race ADC guards at `635-636`, `669-670`, `878-880`.  
Race mode checks ADC faults, but `performAutoSweep()` keeps spinning even when `sensor.adcFault()==true`. Fault-injected mock reproduces full **900 ms** before motor stop.  
**Action:** Check `sensor.adcFault()` after every sweep sample (and ideally before activating motors); stop motors, show SENSOR ADC FAULT, abort immediately. Check fault after each manual scan too. This is especially important because **`LFR_ALLOW_RACE_START=0` does NOT disable motor check or auto-sweep**.

**F06 -- ADC initialization guard expiry is silently accepted**  
Source: `LFR_PRO_V3/LFR_Sensor.cpp:32-50`, `55-62`.  
The reset/calibration loops count down but do not check whether `ADC_CR2_RSTCAL` and `ADC_CR2_CAL` cleared. `adcFault_` only latches when waiting for EOC in `convert()`; a startup calibration timeout may not be caught. **Source-confirmed and injected in mock.**  
**Action:** Explicitly validate both completion flags and latch the initialization error; prevent arming and auto-cal. Consider clock/config validity and meaningful timeout accounting. The actual frequency of such faults is unknown.

**F07 -- Two EEPROM slots are on the same flash page; no power-fail atomicity**  
Source: `LFR_PRO_V3/LFR_Storage.cpp:30-34`, `110-123`.  
The V3.3 one-buffer/one-flush approach addresses excessive flash erasures versus the old per-byte `EEPROM.put()` implementation. But a power cut while the shared flash page is erased/programmed can corrupt **both** copies. CRC can detect bad records but cannot restore lost contents. Host mock asserts a flush counter; it does **not** model torn flash writes.  
**Action:** For true resilience use physically separate erase pages with an atomic append/commit protocol, or external FRAM/EEPROM, and test interrupted writes. Avoid unnecessary frequent Save operations. If hardware cannot change, keep backup configuration/calibration and show explicit re-calibration after invalid records.

**F08 -- 10 cm gap vs fixed 180 ms recovery duration**  
Source: `LFR_PRO_V3/LFR_Control.cpp:198-229`.  
A full 10 cm traverse takes 180 ms only at about 0.56 m/s or faster, not counting sensor geometry and line reacquisition. If the actual speed is slower, the system can time out early. **Conditional; not a confirmed physical failure.**  
**Action:** Measure speed with your real 510-530 g robot and gaps, tune the timeout separately from fixed delay, cap blind motion and test repeated 10 cm gap runs.

**F09 -- Tournament switch limit not met by default hardware**  
Source: `LFR_PRO_V3/LFR_Config.h:18-21`, `LFR_PRO_V3.ino:22-27` (4 navigation buttons), and supplied competition rulebook (maximum two switches including power and reset).  
**Action:** Confirm the exact event's final rules with organizers. A competition-compliant physical input layout must be designed; UI code alone does not make 4 fitted switches compliant. Do not disable safety stop merely to reduce switch count.

### P2 / Improvement, tests or physical validation

**F10 -- Direct ADC performance/accuracy still unmeasured**  
Source: `LFR_PRO_V3/LFR_Sensor.cpp:32-81`, `98-105`, `222-223`; `LFR_Config.h:114-119`.  
The register-level approach avoids repetitive `analogRead()` ADC bring-up/calibration, and the BSRR MUX channel table passes all 16 host checks. The estimated 150-200 us full scan is **not** hardware evidence. The `lastScanUs()` number includes classification/filter work, not only ADC conversion. A mock's 70 us figure is just its simulated `delayMicroseconds` sum.  
**Action:** Check raw ADC values on every sensor with `LFR_FAST_SENSOR=1/0`, dummy conversion on/off, black/white surfaces and alternating channels; record actual scan time and full loop worst case on the STM32. PA2 **analog input voltage must remain within its valid voltage range** (do not drive it with 5 V sensor output).

**F11 -- Real watchdog/E-stop are not independent hardware safety**  
Source: `LFR_PRO_V3/LFR_PRO_V3.ino:814-825`, `871-896`; `LFR_Config.h:81-82`.  
BACK is raw-polled by firmware and 30 ms loop watchdog is software; a halted CPU or hung peripheral cannot rely on polling. The motor check and auto-cal can drive motors with the race lock set to zero.  
**Action:** Raised-wheel E-stop test, accessible motor power cut, watchdog reset/enable design and PWM default safe states; do not treat PWM=0 as guaranteed instantaneous mechanical braking.

**F12 -- `SOURCE_CHANGES.patch` is stale for V3.3**  
Source: archive `SOURCE_CHANGES.patch`; no `eeprom_buffer_fill`, `LFR_ADC_CHANNEL`, `lastScanUs` or new direct ADC changes are present.  
**Action:** Replace or clearly label it as V3.2 patch; export new V3.3 diff from commit `90a475e` to `2bb762d`.

**F13 -- No full event-specific checkpoint representation**  
Source: `LFR_PRO_V3/LFR_Config.h:122-124`, `LFR_Control.cpp:57-68`.  
The script holds 20 L/S/R/U entries, but it does not verify physical checkpoint traversal. Rulebook permits route choice, but not skipping checkpoints.  
**Action:** Map actual course; define checkpoint markers only where physically distinguishable; do not infer checkpoint completion solely from junction count.

**F14 -- 333 Hz control target not yet verified; 1 kHz premature**  
Source: `LFR_PRO_V3/LFR_Config.h:81-82`; `LFR_PRO_V3/LFR_PRO_V3.ino:871-896`.  
`DT_MAX_US` measures control-step spacing and `LOOP_MAX_US` covers sensor+control+drive but not every possible interrupt/wait. Before changing to 1 kHz, measure worst-case latency, sensor scan variance and motor response, then compare lap success and overshoot at 333, 500, 1000 Hz. No 1 kHz or Kakarobot-style turning algorithm is added in this V3.3 foundation.

## Changed-module acceptance assessment

| Module | What is right | What remains |
| --- | --- | --- |
| Sensor.cpp/h | Real register ADC route, MUX lookup, optional dummy conversion, fault latch on missing EOC, measured read duration | Init calibration failures not latched, no real ADC/settling measurement, fallback analogRead no ADC failure detection |
| Config.h | Correct 41.5-cycle ADC setting in source; SAFE/BALANCED timing explicitly set | Clock/voltage and profile tuning are physical assumptions, not measured |
| Storage.cpp | Buffered page programming; CRC and versioned records retained | Both slots share one erase page; mock flash flush is not a power-cut test |
| Sketch .ino | Mirror auto-sweep fixed, race ADC gate, telemetry/timing, time past 99 s | Auto-sweep missing ADC safety abort; WHITE-line black-pad start; 4-button contest constraint |
| Controller.cpp/h | Existing software state/recovery and host regressions retained | Marker vs junction ambiguity, script commit timing, finish ambiguity, gap timeout |
| PID.cpp/h | Bounded filtered D, dt and anti-windup; alpha-beta off | Physical gain/overshoot testing still necessary |
| Motor.cpp/h | Per-time slew, forward/reverse interlock, PWM cap, default deadband off | Actual tire grip, motor braking, stiction, battery and driver heat not measured |
| OLED.cpp/h | UI host snapshots and smoke tests pass | In-field readability/I2C failures not physically measured |

## Go / no-go recommendation

- **GO**: Preserve V3.3 as a *test branch*; source integrity, previous regressions and direct ADC host testing look good.
- **GO, wheels lifted**: Bench checks with `LFR_ALLOW_RACE_START=0` (note motor check/auto sweep remain active), test accurate ADC and emergency stop.
- **NO GO**: Claim 100% bug-free, enable 1 kHz, maximum pivot power or declare full competition readiness before hardware/track tests.
- **Prioritize before Phase 2**: F05 and F06 ADC safety, then F01-F04 track/navigation correctness, F03 line polarity handling, F07 storage risk acknowledgement/mitigation, and F09 physical switch compliance.

## Required real-hardware acceptance checklist

1. Confirm software version, board target and full STM32duino 3.0.0 build for FAST SENSOR `0`, `1`, and `1 + DUMMY READ` with code-size output; store complete logs.
2. With wheels raised, measure PA2 voltage when sensor MUX channels change; never exceed MCU analog pin input limits.
3. Compare **14 raw values**, black and white, across sensor modes. Verify physical left-to-right order, mirror, calibration, and filter stability; measure repeated `SCAN_US` after 100+ scans.
4. Confirm ADC power-on calibration and fault path, including unplugged/invalid MUX behavior. Test auto-sweep ADC failure **after fixing F05**.
5. Back up settings; save, reboot, compare CRC/load sequence, verify legacy migration; do not perform destructive power-cut tests on the only copy of calibration.
6. With wheels raised, check left/right forward/reverse, PWM=0, BACK button hold behavior, power stop and accidental motor-check while race lock is 0.
7. On a safe test track run 10+ slow repetitions each of 3cm line, 90-degree turn, connected T/cross, 10cm gap, full-black 30cm start/finish, and white-line/black-background if applicable. Log `DT_MAX_US`, `LOOP_MAX_US`, `SCAN_US` and faults.
8. Check competition dimensions (15-25 cm length and width, max 25 cm height), 510-530g mass, supply max 24 V, and maximum two physical switches **including reset/power**.
9. Only then benchmark predictive braking, arc vs pivot, and higher control loop rates. Preserve a known-good fallback firmware.

## Reproducing this audit

- Use the untouched uploaded source root and the scripts `sh tests/run_tests.sh`, `sh tests/run_v32_regressions.sh`, `sh tests/run_v33_tests.sh` (run in a COPY because UI tests overwrite `tests/previews/`).
- Additional reproduction C++ sources are supplied separately in `LFR_V33_AUDIT_REPRO_TESTS.zip`. They use the existing mock Arduino API; none are applied to or replace the firmware.
- The `LFR_V33_V32_TO_V33_FIRMWARE.patch` file is exported from the embedded Git commit; it is documentation, not an applied change.

**Important:** No claim of physical speed, sensor precision or reliable hardware safety is made by this report.
