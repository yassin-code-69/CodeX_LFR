# Sensor calibration and setup

Calibration stores a white and a black reading for each of the 14 sensors in the STM32's internal flash. Racing is refused until a valid calibration exists (header shows `NO CAL`).

## Manual calibration (recommended, motors do not move)

1. Menu **WHITE CAL**: place the whole sensor bar on the white surface, press **ENTER**. Message: `SURFACE CAPTURED`.
2. Menu **BLACK CAL**: place the whole bar on black, press **ENTER**.
3. After both: `CAL VALID 14/14` = OK. `BAD W/B CONTRAST` = every sensor needs black at least 250 ADC counts above white; check sensor height (10 mm reported) and surfaces, then repeat both scans.
4. Menu **SAVE SETTINGS** -> `SAVED TO FLASH`. Power-cycle and confirm the header shows `CAL`.

## AUTO calibration (motors move!)

On the WHITE CAL or BLACK CAL screen press **UP**: the robot wiggles left/right for about 0.9 s over the line and records min/max per sensor. Wheels on the track, BACK aborts. An ADC fault stops the motors at once and stores nothing. Then SAVE SETTINGS.

## Check the result

**SENSOR LIVE** shows 14 bars (left to right), the line position, the number of active sensors and the scan time in the footer (for example `150US NORM`). Move a line under the bar: the bars and `POS` must follow from left (negative) to right (positive). If they are reversed, set **MIRROR** = REVERSED (affects reading order only; calibration stays per physical sensor). `ADC ERR` in the header means the ADC failed: do not race; try a build with `LFR_FAST_SENSOR 0`.

## Other settings (menu)

| Item | Values | Start with |
|---|---|---|
| SPEED | SAFE, BALANCED, FAST, SPRINT, TECHNICAL (each loads its PWM/PID preset) | SAFE |
| PID TUNING | base/max/curve/turn PWM, Kp, Ki, Kd, accel/brake ramp, junction time | preset values |
| LINE | BLACK line on white / WHITE line on black | track colour |
| NAV | STRAIGHT, LEFT, RIGHT (hand rule), SCRIPT | STRAIGHT |
| ROUTE EDIT | up to 20 steps S/L/R/U for SCRIPT | as needed |
| GAP | RECOVER (bounded blind crossing) / STOP | RECOVER |
| FINISH | OFF / ON (automatic finish on a sustained full marker) | OFF until the finish rule is known |
| MIRROR | NORMAL / REVERSED | NORMAL |
| TURN MODE | CLASSIC / FAST (not saved; CLASSIC after every power-up) | CLASSIC |

Changes are kept in RAM until **SAVE SETTINGS**; **RESET DEFAULTS** restores SAFE defaults but keeps the calibration. Ranges and compile-time constants: `CONFIG_PARAMETERS.md`.
