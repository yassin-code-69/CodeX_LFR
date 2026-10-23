#!/bin/sh
set -eu
cd "$(dirname "$0")"
CXX=${CXX:-g++}
DIR=${TMPDIR:-/tmp}/lfr-v34-tests
mkdir -p "$DIR"
FLAGS='-std=c++17 -Wall -Wextra -Werror -O2 -Istubs -I../LFR_PRO_V3'
# Phase A navigation: T/+ crossings, side branch, corner, pads, finish, route
# transaction, gap budget, WHITE-mode black start pad (production Sensor+Controller).
$CXX $FLAGS stubs/mock_arduino.cpp ../LFR_PRO_V3/LFR_PID.cpp \
  ../LFR_PRO_V3/LFR_Control.cpp ../LFR_PRO_V3/LFR_Sensor.cpp \
  test_v34_navigation.cpp -o "$DIR/test_v34_navigation"
"$DIR/test_v34_navigation"
# Phase A safety through the real sketch with the direct STM32F1 ADC model and
# fault injection (dead / dying ADC, stuck calibration, corrupt flash).
$CXX $FLAGS -DSTM32F1xx -include stubs/mock_stm32f1.h -DLFR_ALLOW_RACE_START=1 \
  stubs/mock_arduino.cpp ../LFR_PRO_V3/LFR_PID.cpp ../LFR_PRO_V3/LFR_Control.cpp \
  ../LFR_PRO_V3/LFR_Sensor.cpp ../LFR_PRO_V3/LFR_Motor.cpp \
  ../LFR_PRO_V3/LFR_Storage.cpp ../LFR_PRO_V3/LFR_OLED.cpp \
  test_v34_safety.cpp -o "$DIR/test_v34_safety"
"$DIR/test_v34_safety"
