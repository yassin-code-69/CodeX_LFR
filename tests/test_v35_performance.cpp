#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "Arduino.h"
#include "LFR_Config.h"
#include "LFR_Control.h"
#include "LFR_Motor.h"
using namespace lfr;
// V3.5 FAST turn phases and CLASSIC fallback (host simulation of sensor frames).
// These check state logic and command shape only; physical turn quality is UNTESTED.
static const uint32_t DT=3;
static SensorFrame line(int16_t pos=0) {
  SensorFrame f={};f.pos=pos;f.seen=true;f.active=2;f.confidence=850;f.groups=1;
  f.center=pos>-1700 && pos<1700;f.leftEdge=pos<-4450;f.rightEdge=pos>4450;return f;
}
static SensorFrame none() {return SensorFrame{};}
static SensorFrame fan(int8_t side) {
  SensorFrame f=line(int16_t(side*3000));f.center=true;f.active=8;
  f.leftEdge=side<0;f.rightEdge=side>0;f.confidence=1000;return f;
}
static Command tick(Controller &c,const SensorFrame &f,const Settings &s,uint32_t &t) {
  t+=DT;return c.tick(f,s,t,DT*0.001f);
}
static Settings fastProfile() {Settings s=defaultSettings();setProfile(s,1);return s;}  // BALANCED
static void cornerApproach(Controller &c,const Settings &s,uint32_t &t,TurnMode m) {
  c.setTurnMode(m);c.start(0,0);t=0;
  for(int i=0;i<60;++i) tick(c,line(),s,t);
  for(int i=0;i<8;++i) tick(c,fan(-1),s,t);  // left 90-degree corner
}
static void testClassicUnchanged() {
  Settings s=fastProfile();Controller c;uint32_t t;
  cornerApproach(c,s,t,TurnMode::Classic);
  Command cmd=tick(c,none(),s,t);
  assert(c.state()==DriveState::Pivot && cmd.left<0 && cmd.right>0);
  for(int i=0;i<30;++i) tick(c,line(-2400),s,t);  // early-catch zone: ignored in CLASSIC
  assert(c.state()==DriveState::Pivot);
  puts("PASS CLASSIC: corner pivots at once, no BRAKE/CATCH phases");
}
static void testFastCornerPhases() {
  Settings s=fastProfile();Controller c;uint32_t t;
  cornerApproach(c,s,t,TurnMode::Fast);
  Command cmd=tick(c,none(),s,t);
  assert(c.state()==DriveState::Brake && cmd.left==0 && cmd.right==0);
  const uint32_t braked=t;
  while(c.state()==DriveState::Brake) tick(c,none(),s,t);
  assert(t-braked<=LFR_FAST_BRAKE_MAX_MS+DT && c.state()==DriveState::Pivot);
  for(int i=0;i<30;++i) tick(c,none(),s,t);           // rotating, nothing seen
  tick(c,line(-4300),s,t);assert(c.state()==DriveState::Pivot);   // still far out
  cmd=tick(c,line(-2400),s,t);                          // line sweeps in from the left
  assert(c.state()==DriveState::Catch && cmd.left>0 && cmd.right<0);  // opposite pulse
  while(c.state()==DriveState::Catch) tick(c,line(-1500),s,t);
  assert(c.state()==DriveState::Follow);
  int peak=0;
  for(int i=0;i<10;++i) {cmd=tick(c,line(),s,t);peak=cmd.left>peak?cmd.left:peak;}
  assert(peak<s.basePWM);                               // exit ramp still limiting
  for(int i=0;i<80;++i) tick(c,line(),s,t);
  cmd=tick(c,line(),s,t);assert(cmd.left>=s.basePWM);    // ramp released
  puts("PASS FAST: BRAKE (bounded) -> PIVOT -> early CATCH with opposite pulse -> ramped exit");
}
static void testFastGuards() {
  Settings s=fastProfile();Controller c;uint32_t t;
  cornerApproach(c,s,t,TurnMode::Fast);
  tick(c,none(),s,t);assert(c.state()==DriveState::Brake);
  tick(c,line(),s,t);assert(c.state()==DriveState::Follow);  // false corner cancels
  cornerApproach(c,s,t,TurnMode::Fast);
  while(c.state()!=DriveState::Pivot) tick(c,none(),s,t);
  for(int i=0;i<30;++i) tick(c,none(),s,t);
  tick(c,line(2400),s,t);assert(c.state()==DriveState::Pivot);  // wrong side: no catch
  Settings slow=defaultSettings();Controller d;          // slow approach: no brake needed
  d.setTurnMode(TurnMode::Fast);d.start(0,0);t=0;
  for(int i=0;i<3;++i) tick(d,line(),slow,t);
  tick(d,line(-5200),slow,t);tick(d,line(-5200),slow,t);
  assert(d.state()==DriveState::Pivot);
  puts("PASS FAST guards: false corner cancels BRAKE, wrong-side line is not caught, slow entry pivots directly");
}
static void testFastCommitsScriptTurn() {
  Settings s=fastProfile();s.navMode=uint8_t(NavMode::Script);s.routeCount=1;
  s.route[0]=uint8_t(RouteStep::Left);
  Controller c;c.setTurnMode(TurnMode::Fast);c.start(0,0);uint32_t t=0;
  SensorFrame j=line();j.junction=true;j.leftEdge=true;j.active=8;
  for(int i=0;i<20;++i) tick(c,line(),s,t);
  for(int i=0;i<4;++i) tick(c,j,s,t);
  while(c.state()==DriveState::JunctionAdvance) tick(c,j,s,t);
  assert(c.state()==DriveState::Pivot && c.routeUsed()==0);
  for(int i=0;i<40;++i) tick(c,none(),s,t);
  tick(c,line(-2000),s,t);
  while(c.state()==DriveState::Catch) tick(c,line(-1000),s,t);
  assert(c.state()==DriveState::Follow && c.routeUsed()==1);
  puts("PASS FAST early catch commits the SCRIPT turn only after the line is back");
}
// Long seeded stress: ~5 simulated minutes per mode of random frames.
static uint32_t seed=0x2F6E2B1U;
static uint32_t rng(){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void testStress() {
  Motors m;m.begin();
  for(int mode=0;mode<2;++mode) {
    uint32_t restarts=0,ticks=0;
    for(int trial=0;trial<400;++trial) {
      Settings s=defaultSettings();setProfile(s,uint8_t(rng()%5));
      s.navMode=uint8_t(rng()%3);s.finishOn=uint8_t(rng()%2);
      Controller c;c.setTurnMode(mode?TurnMode::Fast:TurnMode::Classic);c.start(0,0);
      uint32_t t=0,phaseStart=0;DriveState last=c.state();++restarts;
      for(int i=0;i<250 && c.active();++i) {
        SensorFrame f={};const unsigned k=rng()%30;
        f.pos=int16_t(int(rng()%13001)-6500);f.seen=k>2;f.active=f.seen?uint8_t(1+rng()%14):0;
        f.center=f.seen && f.pos>-1800 && f.pos<1800;f.leftEdge=f.seen && f.pos<-3900;
        f.rightEdge=f.seen && f.pos>3900;f.groups=f.seen?uint8_t(1+rng()%3):0;
        f.junction=k==3;f.fullMarker=k==4;f.fullBlack=k==4;
        f.confidence=f.seen?uint16_t(250+rng()%751):0;
        Command cmd=tick(c,f,s,t);++ticks;
        assert(cmd.left>=-int(s.maxPWM) && cmd.left<=int(s.maxPWM));
        assert(cmd.right>=-int(s.maxPWM) && cmd.right<=int(s.maxPWM));
        if(c.state()!=last) {last=c.state();phaseStart=t;}
        if(last==DriveState::Brake) assert(t-phaseStart<=LFR_FAST_BRAKE_MAX_MS+DT);
        if(last==DriveState::Catch) assert(t-phaseStart<=LFR_FAST_COUNTER_MS+DT);
        if(!mode) assert(last!=DriveState::Brake && last!=DriveState::Catch);
        m.drive(cmd,s,true,DT*0.001f);
        assert(!(mockPwm[PB8]>0 && mockPwm[PB9]>0) && !(mockPwm[PA0]>0 && mockPwm[PA1]>0));
      }
      m.stop();
    }
    printf("PASS stress %s: %lu races, %lu ticks (%lu s simulated), PWM bounds, interlock, bounded phases\n",
           mode?"FAST":"CLASSIC",(unsigned long)restarts,(unsigned long)ticks,(unsigned long)(ticks*DT/1000));
  }
}
int main() {
  for(int i=0;i<64;++i) mockDigital[i]=HIGH;
  testClassicUnchanged();testFastCornerPhases();testFastGuards();testFastCommitsScriptTurn();
  testStress();
  puts("ALL V3.5 PERFORMANCE TESTS PASSED");
  return 0;
}
