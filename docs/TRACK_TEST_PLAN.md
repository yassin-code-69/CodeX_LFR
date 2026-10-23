# Bench and track test plan (GENGRIX-X1, active branch, baseline V3.5.2)

Rule for every step: **if a wheel moves when it should not, press BACK, cut motor power, stop testing and debug.** Keep the previous verified binaries (`CodeX_LFR_RELEASES/V3.5.1_cf4d560`, `rollback_V3.4.1`) ready to reflash. Record every attempt in the table at the end.

## 0. Before leaving
- Battery 3S charged (note voltage with a meter). Build both firmwares from `firmware/gengrix-x1-active` (see `BUILD_AND_FLASH.md`): RACE_LOCKED with `LFR_ALLOW_RACE_START 0`, and a second build with it set to 1.

## 1. Bench, wheels OFF the ground, race locked (`LFR_ALLOW_RACE_START 0`)
| Step | Expected | Abort if |
|---|---|---|
| Power on | menu, header `NO CAL`/`CAL`, no wheel motion | any motion |
| All 4 buttons | UP/DOWN scroll, ENTER opens, BACK returns | any button dead |
| MOTOR CHECK | L+, R+, L-, R- in order, ~0.4 s each; BACK stops at once | wrong wheel/direction (fix wiring docs, not code) |
| SENSOR LIVE | 14 bars move left->right with a line; footer scan time (expect ~150-250 US, record it) | `ADC ERR` header |
| Reflash with `LFR_FAST_SENSOR 0`, compare raw values on white/black | same within a few % | big differences -> keep 0 |
| WHITE+BLACK calibration (or AUTO, robot on the line), SAVE, power cycle | `CAL` after reboot | lost calibration |
| ARM with race locked | `RACE LOCKED` notice, no motion | motion |

## 2. Floor, race unlocked, SAFE profile, TURN MODE CLASSIC, FINISH OFF
Flash the `LFR_ALLOW_RACE_START 1` build. Optional: `LFR_RACE_TELEMETRY 1` + USB-UART on PA9 to log CSV (columns `T_MS,STATE,PROFILE,TURN,ACTIVE,POS,CONF,FLAGS,JUNC,ROUTE,L_PWM,R_PWM,SCAN_US,LOOP_MAX_US`; STATE numbers: 1 FOLLOW, 2 GAP, 3 PIVOT, 4 JUNCTION, 5 MARKER, 6 FINISHED, 7 FAULT, 8 BRAKE, 9 CATCH; FLAGS bits: 1 seen, 2 centre, 4 left edge, 8 right edge, 16 junction, 32 full marker, 64 full black).

Do each item 5 times; move on only after 5/5:
1. 1 m straight: smooth, no wobble. Record result screen `LOOP xxxUS` (must stay well below 3000).
2. 90-degree left and right: completes, no FAULT. Note overshoot.
3. 30-degree acute left/right: completes within the pivot limit.
4. 15 cm radius curve: stays on the line.
5. 10 cm gap on a straight: crosses. If it FAULTs, time the gap crossing (telemetry) and raise `LFR_GAP_RECOVER_MS`.
6. T and + crossing in STRAIGHT, LEFT, RIGHT nav: correct branch, JUNC count +1 each. From telemetry, read how many ms STATE=5 lasted on the 3 cm crossing; set `LFR_CROSSING_MAX_MS` to ~1.5x that (keep below the 30 cm pad time).
7. Side branch: STRAIGHT passes it, LEFT/RIGHT nav takes it on its side.
8. 30x30 cm black start pad: arm on the pad, drives off onto the line (also in WHITE mode if the track is inverted).
9. Finish pad: FINISH OFF -> stops by timeout (expected). FINISH ON only after the organiser's finish rule is known; confirm it does not stop on crossings.
10. SCRIPT route on a short course: route counter on the result screen equals the junctions taken.

## 3. Speed steps (only after 2 is clean)
BALANCED profile, then TURN MODE FAST (menu item 16, resets to CLASSIC at power-up). For FAST compare 90-degree turn time and overshoot against CLASSIC on the same corner, 5 runs each. Only then TECHNICAL / FAST profiles, raising one parameter at a time.

## 4. Record per attempt
| # | Firmware/profile/turn | Element | Result (OK/FAULT/off-line) | Time | LOOP us | SCAN us | Notes (overshoot, false junction, BACK latency) |
|---|---|---|---|---|---|---|---|
| 1 | | | | | | | |

Ask the organiser about the two-switch rule before the event; the robot keeps its four buttons.
