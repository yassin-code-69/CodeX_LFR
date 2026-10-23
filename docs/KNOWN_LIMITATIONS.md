# Known limitations (baseline V3.5.2)

Nothing below has been tested on the robot. Host tests prove logic on synthetic frames, not physical behaviour.

## Safety
1. **Not flashed, not run.** Boot motor state, BACK latency, motor direction and BTS7960 input behaviour during reset/boot are unverified. Wheels off the ground first.
2. **No independent hardware watchdog.** A HardFault or hang leaves the timers driving the last PWM. The 30 ms loop watchdog and BACK polling are software. (An internal IWDG could be added later; it needs a bench test of BTS7960 inputs during reset.)
3. MOTOR CHECK and AUTO SWEEP move the motors even with `LFR_ALLOW_RACE_START 0`.
4. `LFR_FAST_SENSOR 0` (analogRead) has no ADC fault detection.

## Sensing / navigation
5. **Optical ambiguity.** A 3 cm crossing vs a narrow pad, a finish box vs a wide crossing, a connected T vs a 90-degree corner can be identical; decisions use timing constants (`LFR_CROSSING_MAX_MS` 200 ms, `LFR_FINISH_CONFIRM_MS` 250 ms) that depend on real speed. Tune them from telemetry.
6. STRAIGHT nav turns LEFT at a T (no straight branch). SCRIPT steps that ask for a branch the pattern did not show stop the robot (FAULT) instead of guessing.
7. Side-branch detection needs a clean approach before the branch; on a curve a branch may be missed (counted as nothing), never invented.
8. Route progress is junction based, not checkpoint aware (checkpoints are not optically identifiable).
9. Direct ADC accuracy, crosstalk, scan time (`SCAN_US`) and PA2 voltage range are unmeasured. The U-shaped sensor geometry (per-sensor XY) is unknown and not modelled; position is 1-D.
10. GAP budget 300 ms covers 10 cm only at >= 0.33 m/s; real curve-PWM speed unknown.

## Control / performance
11. FAST turn constants (brake 25 ms per 100 PWM, catch ±2600, catch stop 15 ms, ramp 150 ms) are untuned guesses for a 510-530 g chassis. Speed proxy is commanded PWM, not measured speed.
12. 160 mm axle-to-sensor lookahead changes cornering geometry; junction advance (35 ms) was not re-derived from it.
13. 333 Hz loop not measured on hardware; do not raise the rate before `LOOP_MAX_US` is known.
14. 3S battery voltage drops during a run; timing-based behaviour drifts with voltage (no voltage sensing).

## V3.5.2 notes
- CATCH commands zero PWM and the slew ramps the commanded duty to zero. Zero PWM is **not proven physical braking**: whether the BTS7960 modules short-brake or coast at duty 0 (enable wiring), and how fast a 510-530 g robot stops rotating, are unmeasured.
- SCRIPT stops (FAULT) when the requested branch was not seen during the junction frames. A real branch that the sensors never showed (fast pass, poor contrast, skewed approach) therefore stops the robot instead of turning; confirm on the track.
- A FAST turn whose line vanishes during CATCH stays pending through GAP; it commits only after the line is followed again.

## Storage
15. Both settings slots share one 1 KB flash page: a power cut during SAVE can lose both; the robot then boots on defaults and refuses to arm until recalibrated. Identical saves no longer erase. A two-page journal (ST AN2594 style) is not implemented.

## Rules
16. Four physical buttons vs a reported two-switch rule: needs organiser confirmation; hardware intentionally unchanged.
