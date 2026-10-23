#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "Arduino.h"
#include "LFR_Config.h"
#include "LFR_Control.h"
#include "LFR_Motor.h"
using namespace lfr;
// V3.5.2 safety audit regressions (host simulation). Each scenario was
// reproduced on frozen V3.5.1 cf4d560 before the fix.
static SensorFrame line(int16_t pos=0) {
  SensorFrame f={};f.pos=pos;f.seen=true;f.active=2;f.confidence=850;f.groups=1;
  f.center=pos>-1700 && pos<1700;f.leftEdge=pos<-4450;f.rightEdge=pos>4450;return f;
}
static SensorFrame none() {return SensorFrame{};}
static SensorFrame fan(int8_t side) {
  SensorFrame f=line(int16_t(side*3000));f.center=true;f.active=8;
  f.leftEdge=side<0;f.rightEdge=side>0;return f;
}
static Command tick(Controller &c,const SensorFrame &f,const Settings &s,uint32_t &t) {
  t+=3;return c.tick(f,s,t,0.003f);
}
static Settings script(RouteStep step,uint8_t profile=0) {
  Settings s=defaultSettings();setProfile(s,profile);
  s.navMode=uint8_t(NavMode::Script);s.routeCount=1;s.route[0]=uint8_t(step);return s;
}
// 1. A junction showing only LEFT + STRAIGHT must not be taken to the RIGHT.
static void testScriptBranchMustExist() {
  SensorFrame j=line();j.junction=true;j.leftEdge=true;j.groups=2;j.active=7;
  {Settings s=script(RouteStep::Right);Controller c;c.start(0,0);uint32_t t=0;
   for(int i=0;i<40;++i) tick(c,line(),s,t);
   Command cmd={1,1};for(int i=0;i<4;++i) cmd=tick(c,j,s,t);
   assert(c.state()==DriveState::Fault && cmd.left==0 && cmd.right==0);
   assert(c.routeUsed()==0);}
  {Settings s=script(RouteStep::Left);Controller c;c.start(0,0);uint32_t t=0;  // seen arm: still taken
   for(int i=0;i<40;++i) tick(c,line(),s,t);
   for(int i=0;i<4;++i) tick(c,j,s,t);
   assert(c.state()==DriveState::JunctionAdvance);}
  {Settings s=script(RouteStep::Right);Controller c;c.start(0,0);uint32_t t=0;  // arm seen in an earlier frame
   for(int i=0;i<40;++i) tick(c,line(),s,t);
   SensorFrame jr=j;jr.rightEdge=true;
   tick(c,jr,s,t);tick(c,j,s,t);tick(c,j,s,t);
   assert(c.state()==DriveState::JunctionAdvance);}
  {Settings s=defaultSettings();s.navMode=uint8_t(NavMode::Right);  // non-SCRIPT unchanged
   Controller c;c.start(0,0);uint32_t t=0;
   for(int i=0;i<40;++i) tick(c,line(),s,t);
   for(int i=0;i<4;++i) tick(c,j,s,t);
   assert(c.state()==DriveState::JunctionAdvance);}
  puts("PASS 1: SCRIPT stops (FAULT, zero command, step unused) instead of pivoting into an unseen branch; seen arms and other NAV modes unchanged");
}
// 2. Line lost during FAST CATCH: the turn must not consume the step.
static void testCatchLossKeepsStep() {
  Settings s=script(RouteStep::Left,1);
  SensorFrame j=line();j.junction=true;j.leftEdge=true;j.active=8;
  {Controller c;c.setTurnMode(TurnMode::Fast);c.start(0,0);uint32_t t=0;
   for(int i=0;i<40;++i) tick(c,line(),s,t);
   for(int i=0;i<4;++i) tick(c,j,s,t);
   while(c.state()==DriveState::JunctionAdvance) tick(c,j,s,t);
   for(int i=0;i<40;++i) tick(c,none(),s,t);
   tick(c,line(-2000),s,t);assert(c.state()==DriveState::Catch);
   while(c.state()==DriveState::Catch) tick(c,none(),s,t);
   assert(c.state()==DriveState::Gap && c.routeUsed()==0 && c.routePending());
   while(c.active()) tick(c,none(),s,t);
   assert(c.state()==DriveState::Fault && c.routeUsed()==0);}
  {Controller c;c.setTurnMode(TurnMode::Fast);c.start(0,0);uint32_t t=0;  // line comes back after GAP
   for(int i=0;i<40;++i) tick(c,line(),s,t);
   for(int i=0;i<4;++i) tick(c,j,s,t);
   while(c.state()==DriveState::JunctionAdvance) tick(c,j,s,t);
   for(int i=0;i<40;++i) tick(c,none(),s,t);
   tick(c,line(-2000),s,t);
   while(c.state()==DriveState::Catch) tick(c,none(),s,t);
   for(int i=0;i<10;++i) tick(c,none(),s,t);
   for(int i=0;i<8;++i) tick(c,line(),s,t);
   assert(c.state()==DriveState::Follow && c.routeUsed()==1 && !c.routePending());}
  {Controller c;c.setTurnMode(TurnMode::Fast);c.start(0,0);uint32_t t=0;  // line present: commits at CATCH end
   for(int i=0;i<40;++i) tick(c,line(),s,t);
   for(int i=0;i<4;++i) tick(c,j,s,t);
   while(c.state()==DriveState::JunctionAdvance) tick(c,j,s,t);
   for(int i=0;i<40;++i) tick(c,none(),s,t);
   tick(c,line(-2000),s,t);
   while(c.state()==DriveState::Catch) tick(c,line(-1500),s,t);
   assert(c.state()==DriveState::Follow && c.routeUsed()==1);}
  puts("PASS 2: line lost in CATCH keeps the step pending (GAP timeout -> unused); reacquired line or line at CATCH end commits it");
}
// 3. Real motor outputs (Motors::drive with slew) during CATCH, every profile.
static void testCatchMotorOutputs() {
  for(uint8_t prof=0;prof<5;++prof) {
    for(int i=0;i<64;++i) mockDigital[i]=HIGH;
    Settings s=defaultSettings();setProfile(s,prof);
    Motors m;m.begin();Controller c;c.setTurnMode(TurnMode::Fast);c.start(0,0);uint32_t t=0;
    auto drive=[&](const SensorFrame &f){Command cmd=tick(c,f,s,t);m.drive(cmd,s,true,0.003f);return cmd;};
    for(int i=0;i<300;++i) drive(line());
    for(int i=0;i<8;++i) drive(fan(-1));
    drive(none());
    while(c.state()==DriveState::Brake) drive(none());
    for(int i=0;i<60;++i) drive(none());              // full-speed left pivot
    assert(m.left()<0 && m.right()>0);
    int lastL=m.left(),lastR=m.right(),ticks=0;
    Command cmd=drive(line(-2000));
    while(c.state()==DriveState::Catch) {
      assert(cmd.left==0 && cmd.right==0);           // CATCH commands a stop
      assert(m.left()<=0 && m.right()>=0);            // never reverse (counter) PWM
      assert(m.left()>=lastL && m.right()<=lastR);    // decelerating toward 0
      assert(!(mockPwm[PB8]>0 && mockPwm[PB9]>0) && !(mockPwm[PA0]>0 && mockPwm[PA1]>0));
      lastL=m.left();lastR=m.right();++ticks;
      cmd=drive(line(-1500));
    }
    printf("PASS 3: profile %u (turnPWM %u, brakeStep %u): %d CATCH ticks, wheels %d/%d -> %d/%d, no reverse PWM\n",
           prof,s.turnPWM,s.brakeStep,ticks,-int(s.turnPWM),int(s.turnPWM),lastL,lastR);
  }
}
int main() {
  for(int i=0;i<64;++i) mockDigital[i]=HIGH;
  testScriptBranchMustExist();testCatchLossKeepsStep();testCatchMotorOutputs();
  puts("ALL V3.5.2 SAFETY AUDIT REGRESSIONS PASSED");
  return 0;
}
