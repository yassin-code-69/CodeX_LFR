#include <cassert>
#include <cstdio>
#include <cstring>
#include "Arduino.h"
#include "EEPROM.h"
#define LFR_UI_TEST_ACCESS 1
#include "../LFR_PRO_V3/LFR_PRO_V3.ino"
// V3.3 foundation fixes through the real sketch. Build with -DLFR_ALLOW_RACE_START=1.
#if LFR_ALLOW_RACE_START == 0
#error "build with -DLFR_ALLOW_RACE_START=1"
#endif
static void assertOff() {
  assert(mockPwm[PA0]==0 && mockPwm[PA1]==0 && mockPwm[PB8]==0 && mockPwm[PB9]==0);
}
static Calibration flatCalibration() {
  Calibration c={};
  for(uint8_t i=0;i<SENSOR_COUNT;++i){c.white[i]=160;c.black[i]=3000;}
  return c;
}
static void testProfileJunctionAdvance() {
  Settings s=defaultSettings();
  setProfile(s,2);assert(s.junctionAdvanceMs==25);
  setProfile(s,0);assert(s.junctionAdvanceMs==35);
  setProfile(s,3);setProfile(s,1);assert(s.junctionAdvanceMs==35);
  puts("PASS every speed profile sets its own junction time");
}
static void testRaceTimeFormat() {
  char b[12];
  uiRaceTime(b,5270);assert(std::strcmp(b,"5.27s")==0);
  uiRaceTime(b,99990);assert(std::strcmp(b,"99.99s")==0);
  uiRaceTime(b,123456);assert(std::strcmp(b,"123.45s")==0);
  uiRaceTime(b,2000000);assert(std::strcmp(b,"999.00s")==0);
  puts("PASS race time keeps whole seconds past 99 s");
}
// Every physical channel has its own white/black level; the surface under the
// bar flips every 40 ms while the robot wiggles.
static int sweepSource(int mux) {
  return ((millis()/40U)&1U)?(3000+mux*20):(100+mux*10);
}
static void testAutoSweepMirror() {
  settings=defaultSettings();settings.mirror=1;
  haveCalibration=false;
  mockAdcHook=sweepSource;
  performAutoSweep();
  mockAdcHook=nullptr;
  assert(haveCalibration);
  for(uint8_t i=0;i<SENSOR_COUNT;++i) {
    const int mux=MUX_BY_POSITION[i];
    assert(calibration.white[i]==100+mux*10 && calibration.black[i]==3000+mux*20);
  }
  assertOff();
  settings.mirror=0;
  puts("PASS AUTO CAL with MIRROR REVERSED stores each value at its physical sensor");
}
static void testSingleFlashWritePerSave() {
  const Calibration cal=flatCalibration();
  const int before=mockEepromFlushes;
  assert(saveSettings(settings,cal,true));
  assert(mockEepromFlushes-before==1);
  assert(saveSettings(settings,cal,true));  // other slot, still one page write
  assert(mockEepromFlushes-before==2);
  puts("PASS each SAVE SETTINGS is exactly one flash page write");
}
static void testRaceLoopMeasurements() {
  for(int i=0;i<16;++i) mockMuxADC[i]=160;
  mockMuxADC[6]=3000;mockMuxADC[7]=3000;  // line under the centre sensors
  settings=defaultSettings();
  calibration=flatCalibration();haveCalibration=true;
  sensor.buildNormTable(calibration);
  drawArm();armLaunch();
  assert(page==Page::Countdown);
  for(int i=0;i<40 && page==Page::Countdown;++i) {mockMicro+=50000;loop();}
  assert(page==Page::Race);
  for(int i=0;i<40;++i) {mockMicro+=CTRL_PERIOD_US;loop();}
  assert(page==Page::Race);
  assert(raceMaxIntervalUs>=CTRL_PERIOD_US && raceMaxIntervalUs<CTRL_WATCHDOG_US);
  // Host time only advances in the MUX settle delays of the 14-channel scan.
  assert(raceMaxLoopUs>=uint32_t(SENSOR_COUNT*LFR_MUX_SETTLE_US));
  assert(mockPwm[PB8]>0 && mockPwm[PA1]>0);  // both wheels driving forward
  mockDigital[PB3]=LOW;loop();mockDigital[PB3]=HIGH;
  assert(page==Page::Result);
  assertOff();
  printf("PASS race loop records worst interval %luus and work %luus; BACK stops\n",
         (unsigned long)raceMaxIntervalUs,(unsigned long)raceMaxLoopUs);
}
int main() {
  for(int i=0;i<64;++i) mockDigital[i]=HIGH;
  for(int i=0;i<16;++i) mockMuxADC[i]=160;
  setup();
  assertOff();
  testProfileJunctionAdvance();
  testRaceTimeFormat();
  testAutoSweepMirror();
  testSingleFlashWritePerSave();
  testRaceLoopMeasurements();
  puts("ALL V3.3 FOUNDATION TESTS PASSED");
  return 0;
}
