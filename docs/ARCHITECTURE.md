# GENGRIX-X1 firmware architecture (baseline V3.5.2)

## Data flow (one control step, nominal 3 ms / 333 Hz)

```
loop()
 ├─ BACK (PB3) raw read ──► if pressed in Race/MotorTest/Countdown: motors.stop() first
 ├─ buttons (24 ms debounce) ► UI pages (never while racing except BACK)
 └─ Race page, every >= CTRL_PERIOD_US (wrap-safe micros() difference):
      interval > 30 ms ──► finishRace("LOOP WATCHDOG")
      Sensor::read()      14 x {BSRR MUX select, 5 us settle, ADC1 conversion}
                          -> calibrate (Q16) -> IIR 75/25 -> adaptive threshold
                          -> active/groups/edges/centre/fullMarker/junction/fullBlack
                          -> blob centroid nearest previous position
      adcFault ──► finishRace("SENSOR ADC FAULT")
      BACK ──► finishRace("BACK E-STOP")
      Controller::tick() -> state machine below -> Command{left,right} (signed PWM)
      launch cap (first 140 ms: 75 % of basePWM)
      Motors::drive()     clamp ±maxPWM -> slew (brake/accel steps per 5 ms, scaled by
                          measured dt; reversal always passes through 0) -> BTS7960 PWM
      LOOP_MAX_US / DT_MAX_US bookkeeping; optional CSV telemetry (dropped if UART full)
```
No OLED I2C, no flash writes and no blocking serial in the race loop. Sensor-to-motor latency is one scan plus compute (`LOOP_MAX_US`, measure on the robot).

## Controller state machine

```
start ─► FOLLOW (start-pad phase while fullMarker, or fullBlack && !seen; ≤1500 ms)
FOLLOW ── no line, lastPos extreme or recent fan ─► [FAST: BRAKE ≤60 ms] ─► PIVOT
FOLLOW ── edge-only line (2 frames / instant) ───► [FAST: BRAKE] ─► PIVOT
FOLLOW ── no line, centred ──────────────────────► GAP (≤300 ms RECOVER / 55 ms STOP) ─► FAULT
FOLLOW ── junction pattern x3 / confirmed side branch ─► JUNCTION_ADVANCE ─► FOLLOW or PIVOT
FOLLOW ── fullMarker ─► MARKER
MARKER ── clears ≤ CROSSING_MAX after armed entry ─► intersection decision (T/+) ─► JUNCTION_ADVANCE
MARKER ── finish eligible and ≥ FINISH_CONFIRM ─► FINISHED
MARKER ── clears later ─► FOLLOW (pad)        MARKER ── > 900 ms ─► FAULT
PIVOT  ── centre line 2 frames after min time ─► FOLLOW (commit SCRIPT step)
PIVOT  ── [FAST] line within ±CATCH_POS on turning side ─► CATCH (zero PWM command 15 ms; commanded duty ramps to 0, never reverse; physical braking unverified) ─► FOLLOW + exit ramp
PIVOT  ── > max time ─► FAULT
```
SCRIPT steps are transactions: proposed at the junction, committed on pivot exit / catch (turns) or after 3 clean frames (straight); never while pending; a FAULT leaves the step uncommitted.

Ambiguities that remain by physics: a crossing line and a narrow pad, a finish box and a wide crossing, a connected T and a 90-degree corner can give identical frames. The firmware decides with timing and context, not with certainty.

## Safe fallbacks / one-switch rollbacks

| Feature | Default | Rollback |
|---|---|---|
| FAST turn phases | CLASSIC at every boot | menu TURN MODE = CLASSIC |
| Direct ADC | on | `LFR_FAST_SENSOR 0` |
| Telemetry | off | `LFR_RACE_TELEMETRY 0` |
| Race start | locked | `LFR_ALLOW_RACE_START 0` |
| Automatic finish | OFF | menu FINISH = OFF |
| Whole firmware | - | flash a verified binary from `CodeX_LFR_RELEASES/` (V3.5.1, V3.4.1) or check out `dev/v3.5-performance` / `dev/v3.4-phase-a` |

## Fault handling

ADC init/calibration/conversion failures latch `adcFault`: menu shows `ADC ERR`, arming is refused, AUTO SWEEP and scans stop motors and store nothing, a running race stops. Corrupt or blank flash boots SAFE defaults without calibration, so arming is refused. Every motion state has a time bound (pivot 800-1450 ms, gap 300 ms, marker 900 ms, start pad 1500 ms, brake 60 ms, catch 15 ms, loop watchdog 30 ms). These are software guards only; a hung CPU (HardFault) is not covered by an independent hardware watchdog (see KNOWN_LIMITATIONS.md).
