#!/bin/sh
set -eu
cd "$(dirname "$0")"
CXX=${CXX:-g++}
DIR=${TMPDIR:-/tmp}/lfr-v352-tests
mkdir -p "$DIR"
FLAGS='-std=c++17 -Wall -Wextra -Werror -O2 -Istubs -I../LFR_PRO_V3'
# V3.5.2 safety audit: SCRIPT branch must exist, FAST CATCH line loss keeps the
# step, CATCH motor outputs through the real slew (no reverse PWM, any profile).
$CXX $FLAGS stubs/mock_arduino.cpp ../LFR_PRO_V3/LFR_PID.cpp \
  ../LFR_PRO_V3/LFR_Control.cpp ../LFR_PRO_V3/LFR_Motor.cpp \
  test_v352_safety_audit.cpp -o "$DIR/test_v352_safety_audit"
"$DIR/test_v352_safety_audit"
