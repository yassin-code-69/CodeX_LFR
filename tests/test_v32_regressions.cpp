#include <cassert>
#include <cmath>
#include <cstdio>
#include "Arduino.h"
#include "LFR_Config.h"
#include "LFR_Control.h"
#include "LFR_Motor.h"
#include "LFR_PID.h"
#include "LFR_Sensor.h"
using namespace lfr;
static SensorFrame line(int16_t pos=0) {
  SensorFrame f={};f.pos=pos;f.seen=true;f.active=2;f.confidence=850;
  f.center=pos>-1700 && pos<1700;
  f.leftEdge=pos<-4450;f.rightEdge=pos>4450;
  return f;
}
static SensorFrame full() {
  SensorFrame f=line();f.fullMarker=true;f.active=14;
  f.leftEdge=true;f.rightEdge=true;
  return f;
}
static SensorFrame junc() {
  SensorFrame f=line();f.junction=true;f.active=8;f.leftEdge=true;
  return f;
}
static Settings tuned() {
  Settings s=defaultSettings();
  s.basePWM=175;s.maxPWM=220;s.curvePWM=110;
  s.turnPWM=140;s.accelStep=15;s.brakeStep=50;
  s.kpX10=22;s.kiX100=0;s.kdX100=43;
  return s;
}
static void testRestartTurbo() {
  Settings s=tuned(); Controller c;
  c.start(0,0);
  Command initial=c.tick(line(),s,3,0.003f);
  for(uint32_t t=6;t<=132;t+=3)c.tick(line(),s,t,0.003f);
  c.stop();c.start(300,0);
  Command restarted=c.tick(line(),s,303,0.003f);
  Controller fresh;fresh.start(300,0);
  Command comparison=fresh.tick(line(),s,303,0.003f);
  assert(initial.left==175 && restarted.left==comparison.left && restarted.left==175);
  printf("PASS race restart resets straight turbo: %d == %d\n",restarted.left,comparison.left);
}
static void testReacquireFromJunction() {
  Settings s=tuned(); Controller c;
  c.start(0,0);
  c.tick(line(),s,3,.003f);
  SensorFrame black={};
  c.tick(black,s,6,.003f);
  assert(c.state()==DriveState::Gap);
  c.tick(junc(),s,9,.003f);
  c.tick(junc(),s,12,.003f);
  assert(c.state()==DriveState::Follow);
  c.tick(junc(),s,15,.003f);
  c.tick(junc(),s,18,.003f);
  c.tick(junc(),s,21,.003f);
  assert(c.state()==DriveState::JunctionAdvance);
  assert(c.junctionCount()==1);
  puts("PASS GAP -> junction reacquisition -> exactly one junction");
}
static void testSensorCornerFan() {
  Settings s=tuned(); Calibration cal={};
  for(int i=0;i<14;++i){cal.white[i]=160;cal.black[i]=3000;mockMuxADC[i]=160;}
  for(int i=0;i<8;++i)mockMuxADC[i]=3000;
  Sensor sensor;sensor.begin();SensorFrame f={};sensor.read(f,cal,true,s);
  assert(f.seen && f.center && f.leftEdge && !f.rightEdge);
  assert(f.groups==1 && f.active==8 && !f.junction);
  Controller c;c.start(0,0);
  for(uint32_t t=3;t<=12;t+=3)c.tick(f,s,t,.003f);
  assert(c.routeUsed()==0 && c.junctionCount()==0 && c.state()!=DriveState::JunctionAdvance);
  // Two separated groups have explicit topology unlike a single connected fan.
  for(int i=0;i<14;++i)mockMuxADC[i]=160;
  mockMuxADC[0]=mockMuxADC[1]=mockMuxADC[2]=3000;
  mockMuxADC[5]=mockMuxADC[6]=mockMuxADC[7]=3000;
  sensor.resetTracking();sensor.read(f,cal,true,s);
  assert(f.groups==2 && f.junction);
  puts("PASS connected side fan does not consume junction, separated groups can");
}
static void testStartPadAndFinishOptIn() {
  Settings s=tuned();s.finishOn=1;
  Controller c;c.start(0,0,true);
  for(uint32_t t=3;t<=75;t+=3){c.tick(full(),s,t,.003f);assert(c.state()==DriveState::Follow);}
  c.tick(line(),s,78,.003f);
  assert(c.active() && c.state()==DriveState::Follow);
  for(uint32_t t=81;t<3100;t+=3)c.tick(line(),s,t,.003f);
  const uint32_t at=3102;
  c.tick(full(),s,at,.003f);
  assert(c.state()==DriveState::Marker);
  for(uint32_t t=at+3;t<at+24;t+=3){c.tick(full(),s,t,.003f);assert(c.state()==DriveState::Marker);}
  Command fin=c.tick(full(),s,at+24,.003f);
  assert(c.state()==DriveState::Finished && fin.left==0 && fin.right==0);
  puts("PASS starting full-black pad ignored, explicit finish only after eligible sustained marker");
}
static void testRouteAwareFinishAndGapPad() {
  Settings s=tuned();s.finishOn=1;
  s.navMode=uint8_t(NavMode::Script);
  s.routeCount=1;s.route[0]=uint8_t(RouteStep::Straight);
  Controller c;c.start(0,0);
  for(uint32_t t=3;t<=3000;t+=3)c.tick(line(),s,t,.003f);
  for(uint32_t t=3003;t<=3084;t+=3)c.tick(full(),s,t,.003f);
  assert(c.state()==DriveState::Marker); // route has not been completed
  assert(c.routeUsed()==0);
  puts("PASS scripted route cannot automatically finish before its route steps");

  s.navMode=uint8_t(NavMode::Straight);
  c.start(0,0);c.tick(line(),s,3,.003f);
  SensorFrame absent={};c.tick(absent,s,6,.003f);
  c.tick(full(),s,9,.003f);c.tick(full(),s,12,.003f);
  assert(c.state()==DriveState::Marker);
  for(uint32_t t=15;t<=90;t+=3)c.tick(full(),s,t,.003f);
  assert(c.state()==DriveState::Marker);
  puts("PASS GAP -> full-black patch reacquisition does not invent a finish");

  c.start(0,0,true);
  for(uint32_t t=3;t<=1503;t+=3)c.tick(full(),s,t,.003f);
  assert(c.state()==DriveState::Fault);
  puts("PASS start-pad timeout stops indefinite blind movement");
}
static void testFullBlackFailSafeAndStopPolicy() {
  Settings s=tuned();s.finishOn=0;
  Controller c;c.start(0,0);
  for(uint32_t t=3;t<=30;t+=3)c.tick(line(),s,t,.003f);
  c.tick(full(),s,33,.003f);
  for(uint32_t t=36;t<=753;t+=3)c.tick(full(),s,t,.003f);
  assert(c.state()==DriveState::Marker);
  for(uint32_t t=756;t<=942;t+=3)c.tick(full(),s,t,.003f);
  assert(c.state()==DriveState::Fault);
  puts("PASS marker with FINISH OFF continues through 720ms pad, stops on bounded blind timeout");

  s.gapMode=uint8_t(GapMode::Stop);
  c.start(0,0);c.tick(line(),s,3,.003f);
  SensorFrame empty={}; Command first=c.tick(empty,s,6,.003f);
  Command second=c.tick(empty,s,9,.003f);
  assert(first.left==0 && first.right==0 && second.left==0 && second.right==0);
  c.tick(empty,s,70,.003f);assert(c.state()==DriveState::Fault);
  puts("PASS STOP gap policy commands zero immediately then enters FAULT");
}
static void testMotorPhysicalDutyAndTimeBase() {
  Settings s=tuned();s.accelStep=10;s.brakeStep=30;
  Motors m;m.begin();
  for(int i=0;i<45;i++)m.drive(Command{220,220},s,true,.003f);
  assert(m.left()<=220 && mockPwm[PB8]<=220 && mockPwm[PA1]<=220);
  m.stop();assert(mockPwm[PB8]==0 && mockPwm[PA1]==0);
  m.drive(Command{1,1},s,true,.005f);
  assert(mockPwm[PB8]<=220);
  m.stop();
  int ms3=0;
  while(m.left()<100 && ms3<150){m.drive(Command{100,0},s,true,.003f);ms3+=3;}
  m.stop();
  int ms5=0;
  while(m.left()<100 && ms5<150){m.drive(Command{100,0},s,true,.005f);ms5+=5;}
  assert(std::abs(ms3-ms5)<=5);
  m.stop();
  m.drive(Command{60,0},s,true,.005f);
  assert(!(mockPwm[PB8]>0 && mockPwm[PB9]>0));
  for(int i=0;i<30;i++) {m.drive(Command{-100,0},s,true,.005f);assert(!(mockPwm[PB8]>0 && mockPwm[PB9]>0));}
  puts("PASS physically capped PWM, stop, safe reversal, ~equal 333Hz/200Hz command ramp");
}
static void testPIDRecenterTransient() {
  Settings s=tuned(); PID p;p.reset(0);
  for(int i=0;i<40;i++)p.update(0,.003f,s,220,false);
  for(int i=0;i<3;i++)p.update(4.5f,.003f,s,220,false);
  float late=0;
  for(int i=1;i<=16;i++){
    const float o=p.update(0,.003f,s,220,false);
    if(i==8)late=o;
    assert(std::isfinite(o));
  }
#if LFR_PID_USE_ALPHA_BETA == 0
  assert(fabsf(late)<8.0f);
#endif
  printf("PASS filtered PID transient: 24ms post-center output = %.3f\n",late);
}
int main() {
  for(int i=0;i<64;i++)mockDigital[i]=HIGH;
  testRestartTurbo();testReacquireFromJunction();testSensorCornerFan();
  testStartPadAndFinishOptIn();testRouteAwareFinishAndGapPad();testFullBlackFailSafeAndStopPolicy();
  testMotorPhysicalDutyAndTimeBase();testPIDRecenterTransient();
  puts("ALL V3.2 EXTENDED REGRESSION TESTS PASSED");
}
