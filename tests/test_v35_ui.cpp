#include <cassert>
#include <cstdio>
#include <string>
#include "Arduino.h"
#include "EEPROM.h"
#define LFR_UI_TEST_ACCESS 1
#include "../LFR_PRO_V3/LFR_PRO_V3.ino"
// V3.5 sketch checks. Build with -DLFR_ALLOW_RACE_START=1 -DLFR_RACE_TELEMETRY=1.
#if LFR_ALLOW_RACE_START == 0 || LFR_RACE_TELEMETRY == 0
#error "build with -DLFR_ALLOW_RACE_START=1 -DLFR_RACE_TELEMETRY=1"
#endif
static bool motorsOff() {
  return mockPwm[PA0]==0 && mockPwm[PA1]==0 && mockPwm[PB8]==0 && mockPwm[PB9]==0;
}
static void testTurnModeMenuIsRamOnly() {
  assert(turnMode==TurnMode::Classic);            // boot default
  assert(MENU_ITEMS==16 && menuIndex==0);
  changed=false;
  menuIndex=15;drawMenu();enterMenu();
  assert(page==Page::ModeSelect && modeKind==ModeKind::Turn && modeDraft==0);
  handleButton(2);handleButton(4);                 // BACK cancels the preview
  assert(turnMode==TurnMode::Classic && page==Page::Menu);
  enterMenu();handleButton(2);handleButton(3);     // ENTER applies
  assert(turnMode==TurnMode::Fast && !changed);    // RAM only: nothing to save
  assert(std::string(menuValue(15))=="FAST");
  assert(motorsOff());
  puts("PASS TURN MODE menu item 16: preview/cancel/apply, CLASSIC default, never marks settings for SAVE");
}
static void testTelemetryNeverBlocks() {
  Calibration c={};
  for(uint8_t i=0;i<SENSOR_COUNT;++i){c.white[i]=160;c.black[i]=3000;}
  calibration=c;haveCalibration=true;sensor.buildNormTable(calibration);
  settings=defaultSettings();
  for(int i=0;i<16;++i) mockMuxADC[i]=160;
  mockMuxADC[6]=mockMuxADC[7]=3000;
  drawArm();armLaunch();
  for(int i=0;i<40 && page==Page::Countdown;++i) {mockMicro+=50000;loop();}
  assert(page==Page::Race && controller.turnMode()==TurnMode::Fast);
  const size_t before=Serial1.written;
  for(int i=0;i<40;++i) {mockMicro+=CTRL_PERIOD_US;loop();}
  const size_t sent=Serial1.written-before;
  assert(sent>0);                                  // ~50 Hz CSV rows
  Serial1.room=8;                                  // UART buffer full
  const size_t full=Serial1.written;
  for(int i=0;i<40;++i) {mockMicro+=CTRL_PERIOD_US;loop();}
  assert(Serial1.written==full && page==Page::Race);  // rows dropped, race continues
  Serial1.room=64;
  mockDigital[PB3]=LOW;loop();mockDigital[PB3]=HIGH;
  assert(page==Page::Result && motorsOff());
  printf("PASS telemetry: %lu CSV bytes in 120 ms; full UART drops rows without blocking; BACK stops\n",
         (unsigned long)sent);
}
int main() {
  for(int i=0;i<64;++i) mockDigital[i]=HIGH;
  for(int i=0;i<16;++i) mockMuxADC[i]=160;
  setup();
  testTurnModeMenuIsRamOnly();
  testTelemetryNeverBlocks();
  puts("ALL V3.5 UI/TELEMETRY TESTS PASSED");
  return 0;
}
