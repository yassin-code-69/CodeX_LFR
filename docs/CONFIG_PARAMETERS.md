# Configuration parameters

None of these values is physically tuned. "Measured" means reported by the owner; everything else is a starting point.

## Robot data used by the firmware

| Quantity | Value | Status |
|---|---|---|
| Mass with 3S 1500 mAh battery | 510-530 g | owner-reported |
| Wheel diameter | 35 mm | owner-reported |
| Axle to front-centre sensor | 160 mm | owner-reported (lookahead) |
| Wheel track width | ~175 mm | ESTIMATE from photo; not used in code |
| Sensor ground clearance | 10 mm | owner-reported |
| Motor speed | 25GA 12 V ~1000 rpm rated | spec only; no encoder; not used as speed |

## Runtime settings (OLED menu, saved by SAVE SETTINGS; EEPROM format unchanged)

Speed profiles (unchanged from V3.3, only SAFE/BALANCED junction time made explicit in V3.3):

| Profile | base | max | curve | turn | accel/brake per 5 ms | Kp | Ki | Kd | junction ms |
|---|---|---|---|---|---|---|---|---|---|
| SAFE (default) | 76 | 112 | 52 | 83 | 10/28 | 12.0 | 1.20 | 0.18 | 35 |
| BALANCED | 96 | 150 | 62 | 100 | 12/32 | 14.0 | 0.90 | 0.23 | 35 |
| FAST (experimental) | 165 | 220 | 75 | 125 | 30/45 | 21.0 | 0 | 0.38 | 25 |
| SPRINT (experimental) | 185 | 220 | 90 | 135 | 28/40 | 23.0 | 0 | 0.42 | 22 |
| TECHNICAL | 130 | 190 | 60 | 110 | 18/50 | 19.0 | 0.50 | 0.35 | 30 |

LINE (BLACK/WHITE), NAV (STRAIGHT/LEFT/RIGHT/SCRIPT), ROUTE (20 steps S/L/R/U), GAP (RECOVER/STOP), FINISH (OFF default), MIRROR, PID TUNING: unchanged.

**TURN MODE (V3.5, menu item 16): CLASSIC (default) / FAST.** RAM only: not saved, back to CLASSIC after every power-up, so an experimental mode can never auto-arm from flash.

## Compile-time constants (`LFR_PRO_V3/LFR_Config.h`)

| Constant | Default | Unit | Meaning / how to tune |
|---|---|---|---|
| `LFR_ALLOW_RACE_START` | 0 | flag | race locked until bench checks pass |
| `LFR_FAST_SENSOR` | 1 | flag | direct MUX + ADC1; 0 = analogRead fallback |
| `LFR_SENSOR_DUMMY_READ` | 0 | flag | discard first conversion after MUX switch |
| `LFR_ADC_SAMPLE_CYCLES` | 4 | SMPR code | 41.5 cycles |
| `LFR_RACE_TELEMETRY` | 0 | flag | V3.5 CSV on Serial1 at ~50 Hz, drop-not-block |
| `LFR_CROSSING_MAX_MS` | 200 | ms | all-black shorter than this = T/+ crossing line; longer = pad. Set from telemetry: time a 3 cm crossing at curve PWM, add ~50% |
| `LFR_FINISH_CONFIRM_MS` | 250 | ms | finish needs this long on a full marker (must exceed crossing max; enforced) |
| `LFR_MARKER_TIMEOUT_MS` | 900 | ms | blind-on-black safety stop |
| `LFR_START_PAD_TIMEOUT_MS` | 1500 | ms | start pad must be left within this |
| `LFR_GAP_RECOVER_MS` / `LFR_GAP_STOP_MS` | 300 / 55 | ms | blind gap budget; 10 cm needs >= 0.33 m/s at curve PWM |
| `LFR_APPROACH_HOLD_MS` | 40 | ms | glitch tolerance of the clean-approach history |
| `LFR_FAN_MIN_ACTIVE` / `LFR_FAN_CONFIRM_MS` | 6 / 120 | sensors / ms | side-branch blob width / time for the centred line to reappear |
| `LFR_FAST_BRAKE_MS_PER_100PWM` | 25 | ms per 100 PWM | FAST pre-brake hold vs recent commanded PWM (not measured speed) |
| `LFR_FAST_BRAKE_MAX_MS` | 60 | ms | pre-brake cap |
| `LFR_FAST_CATCH_POS` | 2600 | pos units (1000 = one sensor pitch) | FAST early catch window |
| `LFR_FAST_COUNTER_MS` | 15 | ms | FAST CATCH window: zero PWM command, commanded duty ramps to 0 at brakeStep, never reverse; whether the robot physically brakes is unverified |
| `LFR_FAST_EXIT_RAMP_MS` | 150 | ms | FAST post-turn speed ramp curve -> base |
| `CTRL_PERIOD_US` | 3000 | us | 333 Hz; do not change before measuring LOOP_MAX_US on the robot |
