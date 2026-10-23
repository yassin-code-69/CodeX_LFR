#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "Arduino.h"
#include "LFR_Config.h"
#include "LFR_Control.h"
#include "LFR_Sensor.h"
using namespace lfr;
// V3.4 Phase A navigation regressions (host simulation of sensor frames, 3 ms
// control period). Every scenario reproduces an audit finding on V3.3 first.
static const uint32_t DT=3;
static SensorFrame line(int16_t pos=0) {
  SensorFrame f={};f.pos=pos;f.seen=true;f.active=2;f.confidence=850;f.groups=1;
  f.center=pos>-1700 && pos<1700;
  f.leftEdge=pos<-4450;f.rightEdge=pos>4450;
  return f;
}
static SensorFrame dark() {  // whole bar on the line colour
  SensorFrame f=line();f.fullMarker=true;f.fullBlack=true;f.active=14;
  f.leftEdge=f.rightEdge=true;f.confidence=1000;return f;
}
static SensorFrame none() {return SensorFrame{};}
static SensorFrame fan(int8_t side) {  // connected blob from one edge to centre
  SensorFrame f=line(int16_t(side*3000));f.center=true;f.active=8;f.groups=1;
  f.leftEdge=side<0;f.rightEdge=side>0;f.confidence=1000;return f;
}
static Settings nav(NavMode m) {Settings s=defaultSettings();s.navMode=uint8_t(m);return s;}
static Settings script(std::initializer_list<RouteStep> steps) {
  Settings s=nav(NavMode::Script);s.routeCount=0;
  for(RouteStep r:steps) s.route[s.routeCount++]=uint8_t(r);
  return s;
}
// Feed `f` from t (exclusive) for `ms`; returns the new time.
static uint32_t run(Controller &c,const SensorFrame &f,const Settings &s,uint32_t t,uint32_t ms) {
  for(uint32_t end=t+ms;t<end;) {t+=DT;c.tick(f,s,t,DT*0.001f);}
  return t;
}
static uint32_t approach(Controller &c,const Settings &s) {c.start(0,0);return run(c,line(),s,0,120);}

static void testTCrossingTurnsInsteadOfFault() {
  Settings s=nav(NavMode::Straight);Controller c;
  uint32_t t=approach(c,s);
  t=run(c,dark(),s,t,60);                        // 3 cm crossing line
  assert(c.state()==DriveState::Marker);
  t=run(c,none(),s,t,DT);                        // T: nothing straight ahead
  assert(c.state()==DriveState::JunctionAdvance && c.junctionCount()==1);
  t=run(c,none(),s,t,60);
  assert(c.state()==DriveState::Pivot);
  Command cmd=c.tick(none(),s,t+DT,DT*0.001f);
  assert(cmd.left<0 && cmd.right>0);             // STRAIGHT nav: left at a T
  puts("PASS F01 T crossing (all-black <200 ms, then nothing) is a junction, not GAP->FAULT");
}
static void testScriptAtTAndPlus() {
  {Settings s=script({RouteStep::Right});Controller c;
   uint32_t t=approach(c,s);t=run(c,dark(),s,t,60);t=run(c,none(),s,t,60);
   assert(c.state()==DriveState::Pivot && c.routePending() && c.routeUsed()==0);
   Command cmd=c.tick(none(),s,t+DT,DT*0.001f);assert(cmd.left>0 && cmd.right<0);}
  {Settings s=script({RouteStep::Straight});Controller c;  // no straight at a T
   uint32_t t=approach(c,s);t=run(c,dark(),s,t,60);run(c,none(),s,t,DT);
   assert(c.state()==DriveState::Fault && c.routeUsed()==0);}
  {Settings s=script({RouteStep::Straight,RouteStep::Left});Controller c;
   uint32_t t=approach(c,s);t=run(c,dark(),s,t,60);
   t=run(c,line(),s,t,DT);                       // +: the line continues
   assert(c.state()==DriveState::JunctionAdvance && c.routeUsed()==0);
   t=run(c,line(),s,t,60);
   assert(c.state()==DriveState::Follow && c.routeUsed()==1 && !c.routePending());
   t=run(c,line(),s,t,600);                      // latch and cooldown clear
   t=run(c,dark(),s,t,45);t=run(c,line(),s,t,DT);
   assert(c.junctionCount()==2 && c.routePending());
   t=run(c,line(),s,t,45);
   assert(c.state()==DriveState::Pivot);
   Command cmd=c.tick(line(),s,t+DT,DT*0.001f);assert(cmd.left<0 && cmd.right>0);}
  puts("PASS F01/F04 SCRIPT at T and +: turns as scripted, impossible STRAIGHT stops safely");
}
static void testPadIsNotACrossing() {
  Settings s=nav(NavMode::Straight);Controller c;
  uint32_t t=approach(c,s);
  t=run(c,dark(),s,t,400);                       // 30 cm pad, FINISH OFF
  SensorFrame broad=dark();broad.fullMarker=false;broad.active=9;broad.rightEdge=false;
  t=run(c,broad,s,t,DT);                         // noisy pad frame is not an exit
  assert(c.state()==DriveState::Marker);
  t=run(c,line(),s,t,DT);
  assert(c.state()==DriveState::Follow && c.junctionCount()==0);
  puts("PASS F01 long all-black pad is a pad (no junction), noisy broad frame does not exit it");
}
static void testFinishOptInRobust() {
  Settings s=nav(NavMode::Straight);s.finishOn=1;
  {Controller c;uint32_t t=run(c,line(),s,(c.start(0,0),0),2700);
   t=run(c,dark(),s,t,60);run(c,line(),s,t,DT);  // broad crossing then line
   assert(c.state()!=DriveState::Finished && c.junctionCount()==1);}
  {Controller c;uint32_t t=run(c,line(),s,(c.start(0,0),0),2700);
   SensorFrame glitch=line(2150);glitch.center=false;
   t=run(c,glitch,s,t,DT);                       // one off-centre approach frame
   t=run(c,dark(),s,t,LFR_FINISH_CONFIRM_MS+DT);
   assert(c.state()==DriveState::Finished);}
  puts("PASS F02 FINISH ON: crossing never finishes; one approach glitch no longer loses the finish");
}
static void testSideBranchAndCorner() {
  {Settings s=nav(NavMode::Straight);Controller c;
   uint32_t t=approach(c,s);t=run(c,fan(-1),s,t,30);t=run(c,line(),s,t,DT);
   assert(c.junctionCount()==1 && c.state()==DriveState::JunctionAdvance);
   t=run(c,line(),s,t,60);assert(c.state()==DriveState::Follow);}
  {Settings s=script({RouteStep::Left});Controller c;
   uint32_t t=approach(c,s);t=run(c,fan(-1),s,t,30);t=run(c,line(),s,t,60);
   assert(c.state()==DriveState::Pivot && c.routePending());}
  {Settings s=script({RouteStep::Right});Controller c;  // branch is on the left
   uint32_t t=approach(c,s);t=run(c,fan(-1),s,t,30);run(c,line(),s,t,DT);
   assert(c.state()==DriveState::Fault && c.routeUsed()==0);}
  {Settings s=nav(NavMode::Straight);Controller c;      // 90-degree corner
   uint32_t t=approach(c,s);t=run(c,fan(1),s,t,30);t=run(c,none(),s,t,DT);
   assert(c.state()==DriveState::Pivot && c.junctionCount()==0);
   Command cmd=c.tick(none(),s,t+DT,DT*0.001f);assert(cmd.left>0 && cmd.right<0);}
  {Settings s=nav(NavMode::Straight);Controller c;      // fan without a clean approach
   c.start(0,0);uint32_t t=run(c,fan(-1),s,0,30);run(c,line(),s,t,DT);
   assert(c.junctionCount()==0);}
  puts("PASS F01 connected side branch confirmed only after the centred line resumes; corner pivots to the fan side");
}
static void testRouteTransaction() {
  Settings s=script({RouteStep::Left});Controller c;
  SensorFrame j=line();j.junction=true;j.leftEdge=true;j.active=8;
  uint32_t t=approach(c,s);t=run(c,j,s,t,12);
  assert(c.state()==DriveState::JunctionAdvance && c.routeUsed()==0 && c.junctionCount()==1);
  t=run(c,none(),s,t,1100);                      // pivot never finds the line
  assert(c.state()==DriveState::Fault && c.routeUsed()==0);
  Controller d;t=approach(d,s);t=run(d,j,s,t,12);t=run(d,j,s,t,30);
  assert(d.junctionCount()==1);                  // pending step: no double count
  puts("PASS F04 route step commits only after reacquire; failed turn and repeats do not consume steps");
}
static void testGapBudget() {
  Settings s=nav(NavMode::Straight);Controller c;
  uint32_t t=approach(c,s);t=run(c,none(),s,t,250);   // 10 cm at ~0.4 m/s
  assert(c.state()==DriveState::Gap);
  run(c,line(),s,t,2*DT);
  assert(c.state()==DriveState::Follow);
  Controller d;t=approach(d,s);run(d,none(),s,t,LFR_GAP_RECOVER_MS+2*DT);
  assert(d.state()==DriveState::Fault);
  puts("PASS F08 GAP recovers after 250 ms and still stops after the bounded budget");
}
static void testWhiteModeBlackStartPad() {
  Calibration cal={};
  for(int i=0;i<SENSOR_COUNT;++i){cal.white[i]=160;cal.black[i]=3000;mockMuxADC[i]=3000;}
  Settings s=defaultSettings();s.lineMode=uint8_t(LineMode::White);
  Sensor sensor;sensor.begin();sensor.buildNormTable(cal);
  SensorFrame f={};sensor.read(f,cal,true,s);
  assert(!f.seen && !f.fullMarker && f.fullBlack);
  Controller c;c.start(0,0,true);
  uint32_t t=run(c,f,s,0,600);
  assert(c.state()==DriveState::Follow);
  Command cmd=c.tick(f,s,t+DT,DT*0.001f);assert(cmd.left>0 && cmd.right>0);
  mockMuxADC[6]=160;mockMuxADC[7]=160;                // white line appears
  sensor.read(f,cal,true,s);sensor.read(f,cal,true,s);
  assert(f.seen && f.center);  // 12 sensors may still be black: exit is "line seen"
  run(c,f,s,t,2*DT);assert(c.state()==DriveState::Follow);
  Controller d;d.start(0,0,true);
  for(int i=0;i<16;++i)mockMuxADC[i]=3000;
  sensor.read(f,cal,true,s);run(d,f,s,0,LFR_START_PAD_TIMEOUT_MS+2*DT);
  assert(d.state()==DriveState::Fault);
  puts("PASS F03 WHITE line mode: black start pad drives out to the white line, bounded by timeout");
}
// End to end through the production sensor pipeline: a T on the real sensor model.
static void testSensorLevelT() {
  Calibration cal={};
  for(int i=0;i<SENSOR_COUNT;++i){cal.white[i]=160;cal.black[i]=3000;}
  Settings s=nav(NavMode::Right);
  Sensor sensor;sensor.begin();sensor.buildNormTable(cal);
  Controller c;c.start(0,0);uint32_t t=0;SensorFrame f={};
  auto step=[&](void){t+=DT;sensor.read(f,cal,true,s);c.tick(f,s,t,DT*0.001f);};
  for(int i=0;i<16;++i)mockMuxADC[i]=160;
  mockMuxADC[6]=mockMuxADC[7]=3000;
  for(int k=0;k<40;++k)step();
  for(int i=0;i<16;++i)mockMuxADC[i]=3000;
  for(int k=0;k<20;++k)step();
  assert(c.state()==DriveState::Marker);
  for(int i=0;i<16;++i)mockMuxADC[i]=160;
  step();
  assert(c.state()==DriveState::JunctionAdvance && c.junctionCount()==1);
  for(int k=0;k<20;++k)step();
  assert(c.state()==DriveState::Pivot);
  puts("PASS F01 sensor-level T (real Sensor::read): RIGHT nav takes the T junction");
}
int main() {
  for(int i=0;i<64;++i)mockDigital[i]=HIGH;
  testTCrossingTurnsInsteadOfFault();testScriptAtTAndPlus();testPadIsNotACrossing();
  testFinishOptInRobust();testSideBranchAndCorner();testRouteTransaction();
  testGapBudget();testWhiteModeBlackStartPad();testSensorLevelT();
  puts("ALL V3.4 NAVIGATION REGRESSIONS PASSED");
  return 0;
}
