#include <cstdio>
#include <cstdlib>
#include "Arduino.h"
#include "LFR_Control.h"
#include "LFR_Motor.h"
using namespace lfr;
static SensorFrame line(int16_t pos=0){SensorFrame f={};f.pos=pos;f.seen=true;f.active=2;f.confidence=850;f.groups=1;
 f.center=pos>-1700&&pos<1700;f.leftEdge=pos<-4450;f.rightEdge=pos>4450;return f;}
static SensorFrame none(){return SensorFrame{};}
static SensorFrame fan(int s){SensorFrame f=line(int16_t(s*3000));f.center=true;f.active=8;f.leftEdge=s<0;f.rightEdge=s>0;return f;}
static Command tk(Controller&c,const SensorFrame&f,const Settings&s,uint32_t&t){t+=3;return c.tick(f,s,t,0.003f);}
int main(){
 { // 1: classic junction with LEFT+STRAIGHT only; SCRIPT asks RIGHT
  Settings s=defaultSettings();s.navMode=3;s.routeCount=1;s.route[0]=uint8_t(RouteStep::Right);
  Controller c;c.start(0,0);uint32_t t=0;for(int i=0;i<40;++i)tk(c,line(),s,t);
  SensorFrame j=line();j.junction=true;j.leftEdge=true;j.rightEdge=false;j.groups=2;j.active=7; // centre + left branch
  for(int i=0;i<4;++i)tk(c,j,s,t);
  printf("1: after junction frames state=%s\n",c.status());
  Command cmd={0,0};while(c.state()==DriveState::JunctionAdvance)cmd=tk(c,j,s,t);
  cmd=tk(c,j,s,t);
  printf("1: state=%s cmd L=%d R=%d -> %s\n",c.status(),cmd.left,cmd.right,(cmd.left>0&&cmd.right<0)?"PIVOTS RIGHT into a branch that was never seen (CONFIRMED)":"no right pivot");
 }
 { // 2: FAST catch, line lost during the 15 ms counter interval
  Settings s=defaultSettings();setProfile(s,1);s.navMode=3;s.routeCount=1;s.route[0]=uint8_t(RouteStep::Left);
  Controller c;c.setTurnMode(TurnMode::Fast);c.start(0,0);uint32_t t=0;for(int i=0;i<40;++i)tk(c,line(),s,t);
  SensorFrame j=line();j.junction=true;j.leftEdge=true;j.active=8;
  for(int i=0;i<4;++i)tk(c,j,s,t);while(c.state()==DriveState::JunctionAdvance)tk(c,j,s,t);
  for(int i=0;i<40;++i)tk(c,none(),s,t);
  tk(c,line(-2000),s,t);printf("2: state=%s pending=%d used=%d\n",c.status(),c.routePending(),c.routeUsed());
  while(c.state()==DriveState::Catch)tk(c,none(),s,t);    // line lost during CATCH
  printf("2: after CATCH with no line: state=%s used=%d pending=%d\n",c.status(),c.routeUsed(),c.routePending());
  while(c.active())tk(c,none(),s,t);
  printf("2: final state=%s used=%d -> %s\n",c.status(),c.routeUsed(),c.routeUsed()?"failed turn CONSUMED the step (CONFIRMED)":"not consumed");
 }
 // 3: real motor outputs through Motors::drive during CATCH, per profile
 for(int prof=0;prof<5;++prof){
  for(int longPivot=0;longPivot<2;++longPivot){
   for(int i=0;i<64;++i)mockDigital[i]=HIGH;
   Settings s=defaultSettings();setProfile(s,uint8_t(prof));
   Motors m;m.begin();Controller c;c.setTurnMode(TurnMode::Fast);c.start(0,0);uint32_t t=0;
   auto drv=[&](const SensorFrame&f){Command cmd=tk(c,f,s,t);m.drive(cmd,s,true,0.003f);return cmd;};
   for(int i=0;i<300;++i)drv(line());
   for(int i=0;i<8;++i)drv(fan(-1));
   drv(none());while(c.state()==DriveState::Brake)drv(none());
   const int pivotTicks=longPivot?60:13;            // short: catch right after the 38 ms minimum
   for(int i=0;i<pivotTicks;++i)drv(none());
   int minL=999,maxL=-999,minR=999,maxR=-999,ticks=0;int startL=m.left(),startR=m.right();
   Command cmd=drv(line(-2000));             // this tick enters CATCH: counter command
   while(c.state()==DriveState::Catch){
     ++ticks;
     if(m.left()>maxL)maxL=m.left();if(m.left()<minL)minL=m.left();
     if(m.right()>maxR)maxR=m.right();if(m.right()<minR)minR=m.right();
     cmd=drv(line(-1500));                    // last one leaves CATCH (not recorded)
   }
   (void)cmd;
   const bool reverse=(maxL>0)||(minR<0);  // left pivot: counter torque would be left>0 or right<0
   printf("3: profile %d turnPWM %3d brakeStep %2d %s pivot: wheels at CATCH start L=%4d R=%4d, during CATCH L[%d..%d] R[%d..%d] -> %s\n",
     prof,s.turnPWM,s.brakeStep,longPivot?"long ":"short",startL,startR,minL,maxL,minR,maxR,reverse?"REVERSE PWM delivered":"no reverse PWM (brake toward 0 only)");
  }
 }
}
