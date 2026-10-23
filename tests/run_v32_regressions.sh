#!/bin/sh
set -eu
cd "$(dirname "$0")"
CXX=${CXX:-g++}
DIR=${TMPDIR:-/tmp}/lfr-v32-tests
mkdir -p "$DIR"
FLAGS='-std=c++17 -Wall -Wextra -Werror -O2 -Istubs -I../LFR_PRO_V3'
$CXX $FLAGS stubs/mock_arduino.cpp \
  ../LFR_PRO_V3/LFR_PID.cpp ../LFR_PRO_V3/LFR_Control.cpp \
  ../LFR_PRO_V3/LFR_Sensor.cpp ../LFR_PRO_V3/LFR_Motor.cpp \
  test_v32_regressions.cpp -o "$DIR/test_v32_regressions"
"$DIR/test_v32_regressions"
$CXX $FLAGS stubs/mock_arduino.cpp \
  ../LFR_PRO_V3/LFR_PID.cpp ../LFR_PRO_V3/LFR_Control.cpp \
  ../LFR_PRO_V3/LFR_Motor.cpp \
  test_v32_properties.cpp -o "$DIR/test_v32_properties"
"$DIR/test_v32_properties"
