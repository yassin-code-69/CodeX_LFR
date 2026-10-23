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
