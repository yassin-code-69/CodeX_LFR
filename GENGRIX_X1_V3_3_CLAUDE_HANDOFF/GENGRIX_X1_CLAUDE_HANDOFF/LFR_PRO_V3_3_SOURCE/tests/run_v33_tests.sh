#!/bin/sh
set -eu
cd "$(dirname "$0")"
CXX=${CXX:-g++}
DIR=${TMPDIR:-/tmp}/lfr-v33-tests
mkdir -p "$DIR"
FLAGS='-std=c++17 -Wall -Wextra -Werror -O2 -Istubs -I../LFR_PRO_V3'
# Direct STM32F1 sensor path against a register-level host model.
F1='-DSTM32F1xx -include stubs/mock_stm32f1.h'
$CXX $FLAGS $F1 stubs/mock_arduino.cpp ../LFR_PRO_V3/LFR_Sensor.cpp \
  test_fast_adc.cpp -o "$DIR/test_fast_adc"
"$DIR/test_fast_adc"
$CXX $FLAGS $F1 -DLFR_SENSOR_DUMMY_READ=1 stubs/mock_arduino.cpp \
  ../LFR_PRO_V3/LFR_Sensor.cpp test_fast_adc.cpp -o "$DIR/test_fast_adc_dummy"
"$DIR/test_fast_adc_dummy"
# Foundation fixes through the real sketch (race enabled for the loop test).
$CXX $FLAGS -DLFR_ALLOW_RACE_START=1 stubs/mock_arduino.cpp \
  ../LFR_PRO_V3/LFR_PID.cpp ../LFR_PRO_V3/LFR_Control.cpp \
  ../LFR_PRO_V3/LFR_Sensor.cpp ../LFR_PRO_V3/LFR_Motor.cpp \
  ../LFR_PRO_V3/LFR_Storage.cpp ../LFR_PRO_V3/LFR_OLED.cpp \
  test_v33_foundation.cpp -o "$DIR/test_v33_foundation"
"$DIR/test_v33_foundation"
