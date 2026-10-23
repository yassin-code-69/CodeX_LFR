# GENGRIX-X1 - MASTER ENGINEERING CONTEXT

## Role and Mission

Act as a senior Embedded Systems Engineer, Robotics Control Engineer, firmware architect, safety reviewer, and competition Line-Following Robot specialist. We are developing **GENGRIX-X1**, a high-reliability, high-performance autonomous LFR for competitive racing. Our ambition is to develop exceptionally capable firmware for this exact hardware, with repeatable evidence rather than unverified claims or unnecessary code complexity.

**HARDWARE LOCK - NON-NEGOTIABLE**

No component will be added, removed, replaced, or upgraded in this project. Do not propose an encoder, external EEPROM/FRAM, ESP32, extra sensors, battery monitor module, different motor/driver, or another board as a development requirement. Software-only improvements and empirical tuning of existing hardware are in scope.

### Physical hardware (preserve exactly)

| Part | Confirmed configuration |
| --- | --- |
| Microcontroller | STM32F103C8T6 Blue Pill, STM32duino target |
| Sensor array | 14 analog IR sensors via 16-channel MUX |
| MUX select pins | S0 PA7, S1 PA6, S2 PA4, S3 PA3 |
| ADC signal | PA2, ADC1 channel 2 |
| Left motor interface | BTS7960, PB9/PB8 |
| Right motor interface | BTS7960, PA0/PA1 |
| Motors | Two 25GA 12V 1000 RPM motors |
| Display | SSD1306 OLED |
| Buttons | UP PB13, DOWN PB12, ENTER PB5, BACK PB3 |
| Buzzer | PB4 |
| Weight | 510-530 grams, including battery |
| Wheel encoders | None installed |
| External EEPROM | None installed |
| Persistent settings | Emulated EEPROM stored in STM32 internal Flash |

There is NO physical external EEPROM. STM32duino EEPROM APIs are software interfaces to internal Flash. Power-loss safety and flash wear still matter. Never describe code-level EEPROM changes as newly installed hardware.

Unknown physical measurements (wheel diameter, wheel-to-wheel spacing, tire traction, axle-to-sensor offset, battery pack specification, height, ground clearance) must not be invented. Keep hardware fixed even while investigating these values.

## Source of truth and trust levels

The bundled `LFR_PRO_V3_3_SOURCE/` is the latest user-provided V3.3 Foundation source, originally distributed as CodeX_LFR.rar. Do not assume the public GitHub repository contains these unpushed changes. The existing source files are unchanged in this handoff; `.git` metadata is intentionally excluded. `FILES_SHA256.txt` preserves the upstream content hashes for its listed files. Review actual source before making claims.

Reported tests: existing host suites reportedly pass, and STM32 target compile has been reported with Flash 46,712 B and static RAM 4,024 B. These are prior run/report claims, **not** a substitute for independent reproduction in your own environment. Most critically, physical STM32 flash/bench/track validation has not been completed. Compiling or passing simulated tests does not demonstrate physical performance or safety.

## Existing firmware capabilities

- 14-channel sensor calibration/filtering, dynamic thresholds, position and edge/group classifications
- Direct STM32 ADC1 reading with `LFR_FAST_SENSOR 0` fallback to analogRead path
- Sensor scan and controller loop timing diagnostics
- PID with filtered derivative, anti-windup, optional Alpha-Beta estimator
- Adaptive speed, straight turbo, curvature-based scaling
- Direction-safe motor reversal and time-based acceleration/deceleration
- GAP recovery, PIVOT, FOLLOW, MARKER, JUNCTION_ADVANCE, FAULT/FINISHED states
- OLED mode/configuration/sensor menu, sensor auto-calibration, motor test, countdown
- Script routes and multiple tuning profiles
- Flash-emulated settings storage with CRC and legacy setting migration
- Host regressions and mocked STM32 ADC tests

## Confirmed or serious unresolved risks to fix first

1. Black T and + junction patterns may be classified as `fullMarker` rather than a junction.
2. Connected side branch patterns may be missed by the junction classifier.
3. Script route index can advance before a turn is successfully completed.
4. Ambiguity between all-black intersections and finish markers: avoid unsupported perfect recognition claims.
5. WHITE-line mode and all-black start-pad handling may prevent correct race arming.
6. ADC initialization/reset/calibration failure must reliably latch fault with **bounded** timeouts.
7. ADC faults during motorized auto-sweep calibration must stop motors promptly; prior injection showed a possible long continued sweep.
8. Both settings records may occupy the same Flash erase page; one buffered write reduces wear but does NOT guarantee power-fail atomicity.
9. Fixed GAP timeout needs actual-speed verification for specified 10cm track gaps.
10. Competition rules may cap physical switches at two, whereas current hardware has four buttons. Do not claim firmware alone changes physical switch count; confirm interpretation/permission with organizer.
11. Sensor timing and race-loop worst-case duration must be measured on physical board.
12. The robot has no encoders, so distance/heading estimates from time and motor command are open-loop approximations.

For risks that depend on track shape, design explicit repeatable test patterns and state that some optical patterns are fundamentally indistinguishable from a single sensor frame. Avoid adding imaginary sensors or changing hardware.

## Engineering roadmap

### Phase A - Reliability and Safety

Fix critical detection, route-commit, fault and start/finish defects. Preserve safe motor reversal, conservative race defaults, bounded movement, watchdog checks, and BACK stop handling. Verify calibration and Flash storage. Regression tests for every fix.

### Phase B - Smart Sensor/Track Perception

Confidence-aware centroid, adaptive noise and crosstalk handling, temporal features, curve-vs-sharp-corner event classification, context-aware junction/marker confirmation. Compare any proposal against current weighted sensor behavior; no unmeasured speed promises.

### Phase C - Kakarobot-Inspired Fast Turning

Architecture: DETECT -> PRE-BRAKE -> TURN -> EARLY LINE CATCH -> EXIT -> ACCELERATE. Use controlled differential turns on curves and bounded pivots on truly sharp corners. Keep CLASSIC as default comparison mode. Never assert the competitor's proprietary code has been obtained. Brake-to-pivot thresholds and stop distance need measurements on the 510-530g chassis.

### Phase D - Adaptive Speed

Study curvature- and confidence-aware speed profiles, gain scheduling, straight turbo, motor slew/braking, and loop performance. Starting frequency is ~333Hz (3ms target). Consider 500Hz or 1kHz ONLY after measuring complete worst-case execution time and actual race improvement.

### Phase E - Race Planning

Event-based route execution, route step commit after verified event completion, LEFT/RIGHT/STRAIGHT/U-turn, route mirroring, track configurations, checkpoint-oriented planning where physically observable. Optical sensors cannot independently certify an unmarked checkpoint.

## Competition track scenarios for verification

Use 3cm line width (black on white OR white on black), 30-degree acute angles, 15cm minimum curve radius, 90-degree corners if present, 10cm gaps, T and + intersections, 30x30cm all-black start and finish zones, branching paths, checkpoint completion requirements, and long cave sections where relevant. The event's rulebook governs. Track-dependent cases need track-specific policies; no invented geometry.

## Development and acceptance rules

1. Inspect ALL bundled firmware and test files; understand existing state machine and pin allocation before editing.
2. Make changes incrementally in a development branch or patch, never overwrite the baseline without backup/approval.
3. Prioritize: SAFETY -> RELIABILITY -> NAVIGATION -> CORNERING -> SPEED -> RACE PLANNING.
4. Test both `LFR_FAST_SENSOR=1` and fallback `0`; test NORMAL/REVERSED sensor order, white and black surfaces, and ADC failure injection.
5. Eliminate unbounded motor-running loops and blocking control paths; retain motor PWM caps and safe transitions.
6. Distinguish host simulations, STM32 target builds, and physical hardware validation; present exact logs.
7. Measure `SCAN_US`, `LOOP_MAX_US`, and worst-case `DT_MAX_US` in physical tests. Check ADC crosstalk and pin voltage levels.
8. Preserve/verify saved settings format, CRC, migration, and behavior across reset/power disruption as far as safe.
9. Test 90-degree turn time, line-loss, overshoot, finish detection, gap and junction reliability and full-lap success rates; do repeated before/after tests.
10. Never require ANY new physical component, including an external EEPROM or encoder.
11. Document assumptions, remaining limitations, measured evidence, changes, and rollback method.
12. Do not claim 100% bug-free, guaranteed competition compliance, or guaranteed Kakarobot-level performance without hard data.

## Start request to Claude

Audit the bundled V3.3 source and `INDEPENDENT_V33_AUDIT.md`, prioritize remaining P0/P1 issues, validate the previous findings, propose minimal test-driven corrective patches, and preserve the locked hardware. Provide a patch/diff, tests, STM32 build results if available, and an explicit physical test checklist. DO NOT begin aggressive 1kHz/turbo/fast-pivot implementation until foundational defects have been tested and addressed. Keep original firmware untouched until we review changes.
