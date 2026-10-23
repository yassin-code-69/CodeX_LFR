# GENGRIX-X1 - MASTER ENGINEERING HANDOFF TO CLAUDE
**Date:** 2026-10-23
**Mission:** Finish as much high-quality, tested, modular firmware engineering as possible now, before tomorrow's physical track tests. Preserve existing hardware exactly. Produce complete deliverables, with honest verification boundaries.
**Role:** Act as principal embedded robotics engineer, real-time control engineer, STM32 firmware maintainer, test engineer, and adversarial reviewer. Prioritize completed reliable checkpoints over speculative algorithm count. Do not mistake simulated tests for actual robot results.

## 0. EXECUTION INSTRUCTION: TAKE ACTION, NOT JUST PROVIDE A PLAN
You have the local working codebase and previously began work on `dev/v3.4-phase-a`, based on commit `2bb762d` from `main`. First inspect `git status`, current branch, and actual work already completed; do not assume the branch remains unchanged since the last report. Preserve all useful work. Finish V3.4 Phase A P0 safety/navigation fixes and tests; then continue into V3.5 modular performance firmware on a NEW feature branch from a VERIFIED Phase A checkpoint. Make incremental, logical local commits. Never alter, merge into, reset, force-push, or push `main` without explicit user approval. Do not push any remote branch without permission. Use the available Claude context/agents economically; work from actual evidence, not repeated sprawling analyses. If near limits, leave a complete compilable checkpoint, changelog, unambiguous remaining work list, and usable local ZIP first.

The user cannot do real track tests tonight. DO NOT pretend a physical test was performed. No firmware may claim to have achieved Kakarobot speed or #1 competition performance absent measured evidence. Objective: an advanced candidate ready for safe bench testing and track tuning tomorrow.

## 1. ABSOLUTE HARDWARE LOCK - NO EXCEPTIONS
No new components. No removed components. No upgrades. No rewiring. No pin changes. Four EXISTING buttons must remain physically and logically functional in all operating modes. No external EEPROM or encoder. Preserve backwards-compatible UI and previous settings where feasible. Do not propose adding gyroscope, encoders, voltage/current sensors, radio, camera, extra driver, new regulator, different wheels, or a new MCU. Do not silently claim a battery voltage/current is measured in firmware because no confirmed acquisition path exists.

* MCU: STM32F103C8T6 Blue Pill, nominal 64 KiB program Flash, ~20 KiB SRAM, Arduino_Core_STM32 3.0.0, target 72 MHz.
* Reflectance: 14 analog IR channels routed through multiplexed input. MUX address: PA7, PA6, PA4, PA3; analog signal: PA2 / ADC1 channel 2. Sensor array is curved/U-shaped, with potentially different longitudinal positions; exact per-element XY positions NOT measured.
* Motors: 2 x 25GA 12V ~1000 RPM rated gearmotors, no encoder.
* Motor driver in prior software/hardware specification: dual BTS7960-controlled setup; existing Left PWMs PB9/PB8; Right PWMs PA0/PA1; nominal existing PWM = 10 kHz. Verify source's polarity/driver operations. Photos alone are not enough to identify the actual integrated driver chip, so do not make an unsupported hardware driver claim or change how it is driven.
* OLED: SSD1306 I2C, KEEP existing pins and behavior; do not invent unverified I2C pin mappings.
* Buzzer: PB4.
* Buttons: UP=PB13, DOWN=PB12, ENTER=PB5, BACK/E-STOP=PB3. Preserve all four. PB3 always highest priority E-STOP/abort in active modes.
* Persistent data: software EEPROM emulation in INTERNAL STM32 flash. NO external EEPROM. Respect real erase page boundaries, linker, format, and legacy records.
* Battery: 3S LiPo, 11.1 V nominal, 12.6 V full, 1500 mAh, 30C. Do not assume live battery sensing.
* Robot with battery: 510-530 g; two physical tires may have nonidentical appearance/traction.
* Robot max planform length: 190 mm (front sensor to back, user-reported), width 205 mm. Actual total vertical height NOT established; 10 mm value is reported IR sensor-to-floor clearance, not robot height. A reference photo displayed 5 mm but that was not the actual measured height.
* Wheel outer diameter 35 mm, radius 17.5 mm. L/R inner rubber-tread edge gap 145 mm; wheel tread width ~30 mm each PHOTO ESTIMATE, NOT VERIFIED. Thus track-width/wheel center-to-center estimate b ~175 mm, NOT confirmed. Axle line to front-center sensor user-reported 160 mm; double-check feasibility versus total length before numerical closed-loop tuning. Do not conflate lateral gap with kinematic track width.
* Ignore obsolete measurement strings 3700 mm, 4500 mm, 1000 mm, 'height 29 cm' and the accidentally stated '4.5 cm' wheel gap. They are superseded.
* Competition guidance previously shared: 3-cm line, 10-cm gaps, possible 30-degree acute turns, minimum 15-cm curve radius, black 30x30 cm start/finish areas, intersections, 4/5 checkpoints, roughly 2-minute setup, 5 restarts. Rulebook also reportedly limits switches to 2 including power/reset. Existing four mandatory buttons appear to conflict: FLAG THIS for event organizer; DO NOT pretend firmware alone resolves a physical compliance issue. Never change hardware/button mapping to force compliance. Exact regulations are to be reconfirmed at the event.

## 2. REAL CODEBASE - INSPECT BEFORE EDITING
The original repo is: https://github.com/yassin-code-69/LFR_PRO_V3_1_OLED_UI_FIXED_FULL_CODE/tree/main/LFR_PRO_V3
But remote `main` was older. Local baseline V3.3 `main` was reportedly commit `2bb762d` (independently audit this, do not reset workspace).
Main existing source files: `LFR_Config.h`, `LFR_Control.cpp`, `LFR_Control.h`, `LFR_Motor.cpp`, `LFR_Motor.h`, `LFR_OLED.cpp`, `LFR_OLED.h`, `LFR_PID.cpp`, `LFR_PID.h`, `LFR_PRO_V3.ino`, `LFR_Sensor.cpp`, `LFR_Sensor.h`, `LFR_Storage.cpp`, `LFR_Storage.h`. New test, documentation, simulation, optional modules permitted without disrupting include structure and actual compile.
V3.3 reportedly changed five source files, included direct ADC1 reads under `LFR_FAST_SENSOR`, CRC config slots, mirror calibration fix, 35 ms safe/balanced junction advancement, race display fixes, `LOOP_MAX_US`, `DT_MAX_US`, `SCAN_US`, protected other modules. Earlier V3.3 report: STM32 Blue Pill compile ~46,712B program Flash, 4,024B RAM, 39 host PASS. THESE ARE HISTORICAL USER-REPORTED RESULTS, NOT AN ACCEPTANCE RESULT FOR NEW V3.4/V3.5. Verify independently in this workspace.
Earlier host audit found serious issues. The previous Claude session started auditing F01-F14 with about 10 reviewing agents on `dev/v3.4-phase-a`. Resume existing work, consolidate evidence. Do not discard it.

## 3. RELEASE STRATEGY / BRANCHES
1. `dev/v3.4-phase-a`: complete P0 safety, navigation correctness, ADC failures, route integrity, storage resilience; compile/test; commit as stable SAFE baseline.
2. Create `dev/v3.5-performance` FROM verified V3.4 branch/commit, never directly from old remote or unfinished baseline.
3. Keep features behind compile-time/runtime flags and deterministic profiles: SAFE (default), BALANCED, FAST_EXPERIMENTAL (OFF by default), CLASSIC turn fallback. Performance feature may be compiled and available for tomorrow but must not auto-arm. Save any aggressive settings separately from known-safe calibration. Race Lock ON/default prevents accidental motor activation unless existing UI already has explicitly authorized arming semantics.
4. Changes must be incremental, no giant wholesale rewriting of all 14 files and no unrelated UI redesign. Preserve previous stable control path with one-switch rollback.
5. If 64 KiB Flash is tight, prefer proven P0 and compact modules; remove unused debug strings/fat libraries before adding heavyweight methods. Show full build size & delta. Never exploit undocumented extra program Flash on a nominal C8 target.

## 4. PHASE A P0 - RESOLVE EVERY KNOWN V3.3 FAILURE
A1. Full black / wide active sensor pattern can also signify horizontal T, + cross, checkpoint, black pad, or finish; V3.3's `fullMarker` path could suppress valid junction turns. Distinguish events via contextual state, edge/middle population, temporal progression, route expectation and travel stage. Accept fundamental ambiguity where optics are identical; never invent a perfect classifier.
A2. Scripted route choice step was reportedly consumed inside `choose()` BEFORE completion of the physical turn. Implement transaction: propose -> execute -> confirm reacquired intended line -> COMMIT; abort or rollback on timeout, E-STOP, ADC faults, unexpected event or ambiguous reentry. Prevent double-counting and repeated markers.
A3. False finish on a full-black crossing; missed finish if a short read glitch. Finish must be a debounced, stateful, user-configurable candidate with sensible race context; default automatic finish OFF unless a trusted route/explicit policy exists.
A4. Support black-line/white-line modes including white line mode starting on large black pad without deadlocking arm/start. Distinguish pad from line; allow safe operator arming without motor movement until event.
A5. ADC initial calibration/start timeout may not set fault; motorized auto-calibration can reportedly keep wheels spinning for up to ~900 ms during ADC failure. Bound EVERY ADC init, calibration, conversion, MUX scan, and motion-calibration step; upon fault assert zero motor command immediately, latch fault with audible/UI notice and explicit rearm. E-STOP is honored while calibrating.
A6. Fixed ~180 ms gap recovery can fail at 10 cm depending on speed. Add configurable, bounded gap crossing state with confidence/edge history, no infinite motion and clear fallback; NEVER invent true distance without encoder. If reliable speed estimate unavailable, conservative profile and timeout/stop.
A7. Existing CRC slots reportedly at offsets 512/768 WITHIN ONE MCU FLASH PAGE; CRC on two slots within same erase page is not power-loss atomic. Audit EEPROM library's page boundaries, writes, erase behavior, linker reservation and legacy migration. Either implement physically separate reserved-page transactional scheme with documented verified address safety, or retain existing layout but clearly label weaker guarantees and avoid dangerous migration. No saving while active racing, reduce write frequency; simulate interrupted write/erase and rollback. ST AN2594 describes a two-physical-page emulation pattern.
A8. Audit the other F01-F14 issues found by previous agents, include reproduction and regression tests; don't skip a bug because the exact previous log isn't at hand.

## 5. MOTOR AND FAIL-SAFE CONTRACT
- At boot reset, MCU pending startup, wrong state, invalid sensor/calibration or brownout/reset: safe motor outputs OFF until explicit arming and valid startup checks. Verify active-low/active-high driver semantics in actual existing implementation before deciding hardware output states.
- E-STOP input PB3 highest priority in every state including UI, calibration, gap, turn, recovery; debounce for ordinary UI but don't delay emergency motor stop unnecessarily. Latched STOP requires safe deliberate user action to rearm.
- Motor command API has signed/unsigned documented units, saturating constraints, no inadvertent simultaneous forward/reverse PWM; explicit stop semantics and optional bounded reversal dead-time based on actual driver and existing code. Do not guess active braking response or driver behavior. Track actual commanded PWM and state but don't label commands as measured wheel RPM.
- Strict timeouts and no hangs for calibration, ADC, flash operations, OLED, event recovery; every state transition has bounded duration / condition and fallback. No blocking delay-based high-speed maneuvers.
- Test `micros()` wrap, uint overflow, NaN/inf, divide-by-zero, boot without calibration, invalid saved data, repeated button hits, simultaneous button inputs, prolonged all-black/all-white inputs.
- If battery, motor current, overheating, wheel speed are not wired to firmware, do not add fictitious sensing or closed-loop regulation. MCU brownout behavior is a separate system concern, not a claim to have measured motor voltage.

## 6. HIGH-SPEED SENSOR PIPELINE - MODULAR AND DETERMINISTIC
S1. Keep existing 14-channel mapping, mirrored layout, sensor polarity and calibration compatibility. Build calibrated contrast values [0,1000] with per-channel min/max + validation range and optional hysteresis. Mark low-contrast sensors, saturated/stuck channels, stale scans; return an explicit confidence metric and diagnostic bitset.
S2. Preserve fast direct ADC1 path under feature flag and fallback `analogRead` path. Prove GPIO MUX address settling, acquisition time and no cross-channel memory artifacts in host stimulus tests and bench capture (if real hardware unavailable, label unverified). Every ADC wait bounded. Do not assume 150-200 us full scan unless measured. Keep ADC single owner; no shared library corrupting direct registers.
S3. Provide weighted centroid and robust multi-peak/edge features, left/right wing strength, center presence, active count, width, temporal rate and polarity-normalized comparisons. Do not let one all-black frame override navigation context.
S4. Geometry-aware XY sensor model only if true individual positions are available. Put calibration arrays for per-sensor lateral x and forward y offsets with a safe DEFAULT FALLBACK 1D estimator. Do NOT assign fake x/y by eyeballing photos. Use temporal features initially if true geometry unknown.
S5. Rate-limit/filter fluctuations enough for stability, but no indiscriminate heavy smoothing that adds corner latency. Optional median/EMA/hysteresis with tunable, bounded response. At lost line, return invalid/error confidence explicitly rather than a misleading zero error.
S6. Verify white-line and black-line logic, sensor ordering and mirrored orientation in automated tests. Include no-line, single-peak, double-peak, wide marker, 3-cm line centered/offset, glancing transitions, synthetic sensor noise, stuck channels, sudden brightness change.

## 7. NAVIGATION / EVENT CLASSIFIER AS FINITE STATE MACHINE
Prefer enhancing current state machine and keeping existing APIs, not rewriting all modules for theoretical beauty.
Possible states, integrated appropriately: IDLE, READY, CALIBRATING, ARMED, FOLLOW, PRE_BRAKE, TURN_ALIGN, TURN_ARC, TURN_PIVOT, REACQUIRE, GAP_CROSS, LOST_SEARCH, FINISH_CANDIDATE, FINISHED, FAULT, ESTOP.
Events: ordinary line, mild curve, sharp/acute corner, left-only branch, right-only branch, T, +, dead end, full black area, 10cm gap, marker, sensor fault, lost line. Use hysteresis, min duration/scan count, cool-down and edge transition signatures. When inherently indistinguishable, mark AMBIGUOUS and choose safest policy. Never auto-advance script on an ambiguous or uncompleted maneuver.
- 30-degree acute path, 90-degree left/right, 15cm minimum curve radius, 30mm line, 100mm gap and 300x300mm pads should be represented in host test scenarios (not claimed physically validated).
- Finite turn phases: detect -> PRE_BRAKE -> alignment/lookahead -> fast arc or controlled pivot -> early line catch -> verified reacquisition -> smooth speed ramp.
- Turn direction based on event and script, with LEFT/RIGHT independent trim calibration values if helpful; default symmetrical until measured.
- Implement route transaction commit only on confident post-turn stable line. Delay new junction acceptance until away from same event; configure idempotence/debounce.
- An all-black patch under a single row of sensors cannot conclusively distinguish intersection/finish by itself. Use race context, elapsed time, route count and transitions; default safe route behavior, finish OFF.
- Gap recovery should avoid immediately misclassifying a brief line absence as unrecoverable and avoid unbounded blind driving.

## 8. PERFORMANCE ENGINE - RECOMMENDATIONS WITH VALIDATION GUARDS
P1. Implement actual-dt filtered PD or PID (P+D preferred default, I optionally off or tiny with antiwindup) with fixed arithmetic or efficient floats as benchmarked, derivative filtering and saturation. Normalize line error, tune gains per SAFE/BALANCED/FAST and sensor confidence. Avoid integral windup on no-line/turn/reversal; no NaN.
P2. Use curvature/turn-risk PROXY from error derivative, wing patterns, event confidence and sensor history. A single 1D array cannot directly yield true physical track curvature. Curved XY geometry and ahead sensors may improve prediction when calibrated. Make curvature limits configurable.
P3. Speed command = bounded base target reduced with curvature/instability/event risk and increased only after sustained stable line confidence. Apply acceleration/deceleration ramp with explicit units; optional jerk-limited command trajectory if code-size/runtime allow. Don't raise PWM as first solution. Favor accurate corner completion over straight top speed.
P4. Predictive braking: physically motivated d_stop approximately v*latency + v^2/(2*a) only once velocity and decel are MEASURED/calibrated; otherwise use conservative delay/risk lookup and bench-verifiable profile. User-reported front sensor offset L=160 mm is a tentative preview length, not automatically the free braking distance for every curved sensor and crossing. Do not hard-code numeric braking time from unloaded motor 1000 RPM spec.
P5. Turn modes CLASSIC (default), FAST_ARC, PIVOT with guarded entering speeds and independent timeout, direction and line catch constraints. Controlled motor reversal only if verified and permitted by existing hardware behavior. Fall back to CLASSIC on low confidence.
P6. Segment-aware race profile for known maps: route steps, optional manually configured expected segment duration/turn type/checkpoint; speed up on expected straight, ramp down before expected turn, ALWAYS use real sensor events and bounds to gate decisions. Unregulated 3S voltage makes time-to-distance unreliable; gracefully disable anticipation if inconsistent. No claim of encoder odometry or closed-loop wheel velocity.
P7. Deterministic loop rate options, for example baseline ~250/333 Hz; 500 Hz or 1 kHz only behind option and approved after TRUE measured `SCAN_US`, `LOOP_MAX_US`, jitter and critical section. Do not sacrifice OLED, button or fail-safe liveness. Use `micros()` wrap-safe scheduling, monitor overruns, dt guard; report measured worst case not only average. Maintain existing 10kHz PWM until electrical/thermal verification suggests otherwise.
P8. Optimize all the way from sensor capture to motor command: keep OLED I2C out of critical fast loop (e.g. event-driven/low-rate updates), no blocking `Serial.print`, no Flash writes while driving; optional small bounded telemetry ring buffer or compile-out logging; report CPU/Flash/RAM.
P9. Optional wheel imbalance compensation: separate motor command trim tables/offsets ONLY after user tests. Tire differences in photo are a reason to measure asymmetry, not evidence to pre-bias motor PWM.
P10. Avoid unwarranted 'AI', neural network, reinforcement learning or heavyweight MPC until measurable superiority is demonstrated with appropriate hardware telemetry and available memory. A compact verified control law wins over a complex untestable one.

## 9. USER EXPERIENCE: PRESERVE 4 BUTTONS / OLED
Keep EXACT PB13/PB12/PB5/PB3 semantics and existing navigation, editing/storage if present. Non-blocking UI. Expose only actually useful settings: SAFE/BALANCED/EXPERIMENTAL, line polarity, calibration, turn mode, max PWM, PD gains, brake factor, event thresholds, gap-recovery time, race lock / route script / mirrored orientation, recent fault code and timing diagnostic. Validate ranges and prevent changes while motor active. OLED in critical mode must not stall motor loop. PB3 must always abort; emergency stop should not depend on OLED updating.

## 10. INTERNAL-FLASH SETTINGS CORRECTNESS
Read current EEPROM library implementation and actual STM32F103C8 flash page geometry. Verify current CRC records and flash emulated slots. Ensure program and settings do not overlap after code growth. Ideally implement power-loss-safe physically separate page journal with generation, version, CRC and commit marker, but ONLY if fully verified memory allocation, linker, backward migration and tests; else keep verified backward-compatible scheme and explicitly report residual atomicity risk. Writes ONLY while disarmed/IDLE, no writes per loop or per race frame. Invalid or interrupted writes must never auto-start motors. Provide tests for old schema, new schema, corrupted CRC, blank flash and cut power at every simulated write step.

## 11. HOST TESTS / VIRTUAL TRACK SIMULATION - REQUIRED BEFORE TOMORROW
Use portable host C++ test harness with mocked `millis`, `micros`, sensor ADC/MUX and motors/Flash; ideally deterministic and seeded. No external real hardware assumed. Existing tests from V3.3 must pass unchanged unless intentional behavior change is explained. Create regression tests with explicit assertions for:
T01 startup motor-off and no involuntary auto-calibration spin;
T02 PB3 ESTOP stops any motor command in every active state, is latched;
T03 ADC init timeout, conversion timeout, stale channel, sensor fault -> immediate STOP;
T04 all-14-black event crossing/T/checkpoint/finish candidates are not permanently suppressed;
T05 false full-black finish not triggered while navigating crossing;
T06 finish recognition if explicitly enabled and robust to single read glitch;
T07 SCRIPT transaction: no counter increment before successful line reacquire; abort rollback;
T08 repeated junction and marker debounce prevent double count;
T09 white-line mode starting on black pad can be safely armed;
T10 100mm gap at slow, medium, fast simulated speeds with bounded timeout; no endless driving;
T11 left/right turn success, miss and timeout, gentle and sharp 90/30deg transitions;
T12 curve 150mm radius synthetic sensor trace for control and stability (simulation not proof of physical execution);
T13 mirrored 14-sensor mapping, swapped polarity and per-channel low contrast;
T14 black/white calibration rejection, corruption, zero range;
T15 bounded integral, derivative spikes, dt=0, wraparound, PWM clipping, proper reversal interlock;
T16 route limits/overflow, stale segment plan, unexpected extra checkpoint;
T17 storage legacy migration, CRC corruption, interrupted erase/write and retry;
T18 corrupted settings cause SAFE defaults, race locked;
T19 OLED update and serial logging cannot unboundedly block motor control;
T20 multiple random seeded stress traces, sensor chatter, false branches, 5-minute sustained host simulation without hanging or out-of-bounds.
Also run address sanitizer/UB sanitizer where available, strict warnings, static analysis, random regression seeds, no hidden failing tests. Tests must use actual production logic not a trivial reimplementation. Save results as files. Be precise: 'HOST PASS' != hardware speed/turn verified.

## 12. BUILD AND PERFORMANCE VERIFICATION
- Reproduce STM32 target compile with the EXACT installed/declared Arduino_Core_STM32 3.0.0 board package, correct F103C8 target, libraries and defines. If tooling unavailable, produce reproducible local build instructions AND label STM32 target build UNVERIFIED; do not fabricate compile numbers.
- Verify no errors/major warnings, all modules link, Flash <=64KiB official limit (leave headroom and guard any page reservation), RAM <=20KiB plus stack headroom, runtime timing within chosen baseline loop. Show changed bytes from V3.3. Produce and save .elf/.bin/.hex only if real build tools produced them; make sure firmware is configured to start disarmed.
- Test `LFR_FAST_SENSOR=1` and `=0` where practical, black/white line, mirror on/off, SAFE/EXPERIMENTAL compile flags. Bench performance metrics must be labeled SIMULATED if on host. Do not guess real `SCAN_US`.
- No software change should rely on undefined Arduino behavior or a new library with incompatible platform versions.

## 13. DOCUMENTATION AND OUTPUTS - NONNEGOTIABLE
At the end, place deliverables in a clearly labeled local directory and ZIP, not just chat commentary:
1. FULL UPDATED SOURCE (all buildable firmware files and dependencies, not a cherry-picked patch).
2. `CHANGELOG_V34_V35.md` with every significant change / test / known risk.
3. `ARCHITECTURE.md` including data flow, sensor-to-motor latency, state machine, safe fallback, all feature flags.
4. `PIN_HARDWARE_LOCK.md` before/after table proving unchanged all physical pins/buttons and no component additions.
5. `CONFIG_PARAMETERS.md` SAFE vs BALANCED vs FAST experimental config, ranges, units, default, physical measurements vs estimates; never label defaults as physically tuned.
6. `TEST_RESULTS.md` table for host tests, target compile, real bench, real track, with PASS/FAIL/UNTESTED separated, seed/command/log links.
7. `REMAINING_RISKS.md` with all unresolved safety/ADC/Flash and performance uncertainties.
8. `TOMORROW_UNIVERSITY_TEST_PLAN.md` concise step-by-step battery/motor checks, calibration, wheel-direction validation, low-speed straight, 90 left/right, 30 acute, 150mm curve, 100mm gap, T/+ all black, black start/finish area, checkpoints, SAFE -> BALANCED -> EXPERIMENTAL incrementally. For each: expected behavior, abort criteria, what parameters/serial/OLED diagnostics to record. No new external hardware mandatory.
9. `GIT_DIFF.patch`, branch/commit IDs, build commands and logs, `git status` output. Only create local commits, no pushes or merges without user authorization.
10. Summary scorecard: Completed | Host verified | STM32 build verified | Physical test pending | Blocked. Do not conceal missing physical verification.

## 14. TESTING ORDER TOMORROW (WRITE GUIDE, DO NOT PERFORM NOW)
Test in safe mode with wheels OFF GROUND first for correct left/right polarity, 4 buttons/E-STOP, sensor read/status, fault handling. Use a basic line surface at low PWM, short straight, controlled left & right corner, 10-cm gap, T/cross, start/finish pads, then chained full track. Prepare track widths based on the competition rules. Log result per attempt: state transitions, gap/turn events, lap completion/checkpoint rate, time, E-STOP delay observation, false-marker count, lost-line count, max loop and scan duration. Increase aggression only after repeated complete clean runs. If wheels move unexpectedly at any time, stop tests immediately and debug safety path. Respect competition switch rule issue; organizer confirmation required for fixed 4 buttons.

## 15. ACCEPTANCE AND PRIORITY
P0: safe boot, every-state E-STOP, bounded ADC calibration/fault, script transaction, robust all-black ambiguity handling, no false finish, no stuck motion, host regressions, buildable checkpoint.
P1: adaptive PD, speed planning, fast arc & pivot behind flags, early line catch, curved-sensor data model with honest geometry uncertainty, line/gap recovery, profiling, UI parameters.
P2: track script/learning, advanced Flash journal (only with verified memory safety), high-loop experimental options, compact telemetry and simulation refinements.

In the final answer, report actual changed files and tests, host vs MCU compile distinction, remaining physical testing, known trade-offs, exact branch/commit/checksum of handoff ZIP, and immediate steps for tomorrow. NEVER declare '100% bug-free', 'world fastest', or 'ready at full speed' without evidence. This is a competition-grade engineering effort with fixed hardware and an aggressive software-only improvement goal.

## 16. RESEARCH GUIDANCE (INDEPENDENT PUBLIC SOURCES, NOT SECRET KAKAROBOT CODE)
- Kakarobot official catalog: https://kakarobot.id/products -- a public teaching robot uses PID, ESP32, 8 sensors, but does NOT disclose the racing firm's secret algorithms. Don't fabricate them.
- Pololu 3pi: https://www.pololu.com/docs/0j21/all -- line following, PD/PID, pre-turn slowing, timed route segments and warnings about nonregulated battery.
- Pololu line follower code: https://github.com/pololu/pololu-3pi-plus-32u4-arduino-library/blob/master/examples/LineFollowerPID/LineFollowerPID.ino -- reference principles only, different platform.
- ST STM32F103C8: https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8 -- nominal 64 KiB flash and 72MHz.
- ST AN2594: https://www.st.com/resource/en/application_note/an2594-eeprom-emulation-in-stm32f10x-microcontrollers-stmicroelectronics.pdf -- F10x Flash EEPROM emulation principle, dual actual pages and power fail considerations.
- STM32duino API: https://stm32duino.github.io/dev/API/ -- verify Arduino core ADC resolution and PWM behavior.

## 17. RESPONSE FORMAT: START WITH EXECUTION AND FINISH WITH ARTIFACTS
Start by reporting current git branch/status + preserved baseline in 5-10 lines, then directly implement. Ask only if truly blocked by destructive risk or missing code. You are authorized to read and modify the local working branch, run compilers/test harnesses, and make local feature commits. You are NOT authorized to alter hardware, change pin mapping/buttons, merge into main, or push. Do not waste the limited work window producing repeated planning essays. Preserve a working source ZIP and test results as the highest-value artifact.

## 18. ADDITIONAL VERIFIED ENGINEERING NOTES / NO INVENTED PERFORMANCE
- Pololu's official 3pi tuning notes explicitly distinguish fast straights from safe intersections. It measured segment time during a learning run, slowed before expected turns, and warns that this time-based approximation is fragile without regulated motor power or encoders. Treat this as a transferable principle, NOT a drop-in timing constant: https://www.pololu.com/docs/0J21/8.f
- STM32duino's own EEPROM documentation states that its Flash-backed EEPROM emulation erases/reprograms a whole Flash page/sector and advises buffering saves. Since older CRC slots were reportedly within one actual page, CRC alone does not provide a recoverable second copy after interrupted erase: https://stm32duino.github.io/dev/libraries/built-in/EEPROM-library/
- The official STM32F103C8 is nominally 64 KiB Flash, 20 KiB SRAM, 72 MHz and a 12-bit ADC. Verify all compiled footprint/linker allocation against this device instead of relying on larger Blue Pill clone capacity: https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8
- The Infineon BTS7960 half-bridge datasheet documents internally protected power stages, dead-time generation and a PWM capability up to 25 kHz. That specification does NOT prove the precise assembled dual motor driver module or its wiring, nor does it justify changing current 10 kHz PWM without tests: https://www.infineon.com/assets/row/public/documents/10/57/infineon-bts7960-ds-en.pdf
- Kakarobot publicly lists an educational ESP32/8-sensor/PID line-follower product; this is NOT verified evidence for their fastest racing robot's design, lap times, or proprietary algorithms: https://kakarobot.id/products
- Even a brilliant untested controller cannot guarantee first place. Success requires correct event handling, low latency, controlled corner speed, repeated complete runs, track-specific tuning and solid fault handling. Prefer measured race finish consistency over peak wheel RPM or synthetic speed estimates.

## 19. TELEMETRY FOR PHYSICAL VALIDATION
Expose a compact, bounded, disabled-by-default diagnostic mode suitable for observation tomorrow: timestamp or elapsed ms, robot state, sensor active bitmask, calibrated line error/centroid, sensor confidence, event type, turn phase, LEFT and RIGHT commanded PWM, scan time, worst loop time and fault code. Include a column to indicate the selected SAFE/BALANCED/EXPERIMENTAL profile. Serial or OLED updates must never block the control loop; rate-limit or buffer and drop excessive telemetry. If insufficient pins for existing Serial, prefer existing interface and do not invent any extra connector. Generate a CSV schema/example and a manually fillable lap-test table without claiming telemetry was captured today.

## 20. PRE-RELEASE GATES FOR TOMORROW
- Gate 0: preserve source snapshot; git status and exact base hash; pin-lock audit; detect build infrastructure.
- Gate 1 (V3.4): P0 fixes, full host regressions, real STM32 target build if tools present; keep validated fallback. Produce ZIP/checkpoint before performance work.
- Gate 2 (V3.5): modular sensor+controller+speed planner+guarded fast-turn implementation and tests; SAFE defaults and one-switch disable. Produce separate ZIP/checkpoint.
- Gate 3: final bundle: complete source, logs, diff, docs, commands, checksums and prioritized university tests. If target compile or host tests fail, do not falsely mark release PASS. If token/time constraints arise, stop at the last clean compilable checkpoint and explicitly enumerate deferred items.
- Tomorrow: no full-speed tests until physically verifying stopped motors at boot, fail-safe, direction, calibration and low-speed turn recovery. A software E-STOP cannot be claimed instantaneous if the actual processing architecture and hardware have not been timed.
