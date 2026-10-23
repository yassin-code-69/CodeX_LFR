#include <cassert>
#include <cstdio>
#include <cstring>
#include "Arduino.h"
#include "EEPROM.h"
#define LFR_UI_TEST_ACCESS 1
#include "../LFR_PRO_V3/LFR_PRO_V3.ino"
// V3.4 Phase A safety regressions through the real sketch, with the direct
// STM32F1 ADC path (register mock) and fault injection.
// Build: -DSTM32F1xx -include stubs/mock_stm32f1.h -DLFR_ALLOW_RACE_START=1
#if !defined(STM32F1xx) || LFR_ALLOW_RACE_START == 0
#error "build with -DSTM32F1xx -include stubs/mock_stm32f1.h -DLFR_ALLOW_RACE_START=1"
#endif
static bool motorsOff() {
  return mockPwm[PA0]==0 && mockPwm[PA1]==0 && mockPwm[PB8]==0 && mockPwm[PB9]==0;
}
static Calibration flatCalibration() {
  Calibration c={};
  for(uint8_t i=0;i<SENSOR_COUNT;++i){c.white[i]=160;c.black[i]=3000;}
  return c;
}
static void useCalibration() {
  calibration=flatCalibration();haveCalibration=true;sensor.buildNormTable(calibration);
}
static void runCountdown() {
  for(int i=0;i<40 && page==Page::Countdown;++i) {mockMicro+=50000;loop();}
}
static void testWhiteModeArmsOnBlackPad() {
  settings=defaultSettings();settings.lineMode=uint8_t(LineMode::White);useCalibration();
  for(int i=0;i<16;++i) mockMuxADC[i]=3000;      // 30x30 cm black START pad
  drawArm();armLaunch();
  assert(page==Page::Countdown && motorsOff());  // V3.3: START LINE INVALID
  runCountdown();
  assert(page==Page::Race);
  for(int i=0;i<20;++i) {mockMicro+=CTRL_PERIOD_US;loop();}
  assert(page==Page::Race && mockPwm[PB8]>0 && mockPwm[PA1]>0);  // drives off the pad
  mockDigital[PB3]=LOW;loop();mockDigital[PB3]=HIGH;
  assert(page==Page::Result && motorsOff());
  settings=defaultSettings();
  puts("PASS F03 WHITE mode arms on a black start pad, drives off it; BACK stops");
}
static void testCorruptSettingsBootLocked() {
  std::memset(EEPROM.data,0xFF,sizeof(EEPROM.data));  // blank / torn flash page
  setup();
  assert(!haveCalibration && settings.speedProfile==0 && motorsOff());
  drawArm();armLaunch();
  assert(page==Page::Notice && motorsOff());     // NO CALIBRATION: race refused
  puts("PASS F07 blank/corrupt flash boots SAFE defaults without calibration; arming refused");
}
static void testSweepStopsWhenAdcDiesMidway() {
  useCalibration();settings=defaultSettings();
  mockAdc1Instance.dieAfter=mockAdc1Instance.conversions+14*20;  // ~20 scans in
  const uint32_t before=millis();
  performAutoSweep();
  const uint32_t spent=millis()-before;
  assert(sensor.adcFault() && motorsOff() && page==Page::Notice);
  assert(spent<200);                             // V3.3: full 900 ms of motion
  printf("PASS F05 ADC dies during AUTO SWEEP: motors stopped after %lums (was 900 ms)\n",
         (unsigned long)spent);
}
static void testDeadAdcNeverSpinsOrCalibrates() {
  // adcFault is latched from the previous test: the sweep must not move at all.
  const int writes=mockPwmNonZeroWrites;
  const Calibration kept=calibration;
  performAutoSweep();
  assert(mockPwmNonZeroWrites==writes && motorsOff() && page==Page::Notice);
  scanBlack=false;performScan();scanBlack=true;performScan();
  assert(std::memcmp(&kept,&calibration,sizeof(Calibration))==0);
  drawArm();armLaunch();
  assert(page==Page::Notice && motorsOff());
  puts("PASS F05 latched ADC fault: AUTO SWEEP never drives, manual scans store nothing, arming refused");
}
static void testStuckCalibrationLatchesFault() {
  mockAdc1Instance.calStuck=true;
  Sensor fresh;fresh.begin();
  mockAdc1Instance.calStuck=false;
  assert(fresh.adcFault());
  SensorFrame f={};const int conv=mockAdc1Instance.conversions;
  fresh.read(f,calibration,true,settings);
  assert(mockAdc1Instance.conversions==conv);    // fast-fail, no waiting per channel
  puts("PASS F06 stuck RSTCAL/CAL during ADC init latches the fault; later scans fail fast");
}
int main() {
  for(int i=0;i<64;++i) mockDigital[i]=HIGH;
  for(int i=0;i<16;++i) mockMuxADC[i]=160;
  setup();
  assert(motorsOff() && !sensor.adcFault());
  testWhiteModeArmsOnBlackPad();
  testCorruptSettingsBootLocked();
  testSweepStopsWhenAdcDiesMidway();
  testDeadAdcNeverSpinsOrCalibrates();
  testStuckCalibrationLatchesFault();
  puts("ALL V3.4 SAFETY REGRESSIONS PASSED");
  return 0;
}
