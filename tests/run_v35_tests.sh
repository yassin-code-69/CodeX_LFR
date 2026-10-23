#!/bin/sh
set -eu
cd "$(dirname "$0")"
CXX=${CXX:-g++}
DIR=${TMPDIR:-/tmp}/lfr-v35-tests
mkdir -p "$DIR"
FLAGS='-std=c++17 -Wall -Wextra -Werror -O2 -Istubs -I../LFR_PRO_V3'
# FAST turn phases vs CLASSIC fallback, plus ~5 simulated minutes of seeded
# random frames per mode (PWM bounds, H-bridge interlock, bounded phases).
$CXX $FLAGS stubs/mock_arduino.cpp ../LFR_PRO_V3/LFR_PID.cpp \
  ../LFR_PRO_V3/LFR_Control.cpp ../LFR_PRO_V3/LFR_Motor.cpp \
  test_v35_performance.cpp -o "$DIR/test_v35_performance"
"$DIR/test_v35_performance"
# TURN MODE menu (RAM only) and non-blocking CSV telemetry through the sketch.
$CXX $FLAGS -DLFR_ALLOW_RACE_START=1 -DLFR_RACE_TELEMETRY=1 stubs/mock_arduino.cpp \
  ../LFR_PRO_V3/LFR_PID.cpp ../LFR_PRO_V3/LFR_Control.cpp \
  ../LFR_PRO_V3/LFR_Sensor.cpp ../LFR_PRO_V3/LFR_Motor.cpp \
  ../LFR_PRO_V3/LFR_Storage.cpp ../LFR_PRO_V3/LFR_OLED.cpp \
  test_v35_ui.cpp -o "$DIR/test_v35_ui"
"$DIR/test_v35_ui"
