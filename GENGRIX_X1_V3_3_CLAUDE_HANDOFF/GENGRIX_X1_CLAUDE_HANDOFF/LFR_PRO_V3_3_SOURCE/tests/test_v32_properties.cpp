#include <cassert>
#include <cstdio>
#include <cstdint>
#include "Arduino.h"
#include "LFR_Config.h"
#include "LFR_Control.h"
#include "LFR_Motor.h"
using namespace lfr;
static uint32_t seed=0x5534882U;
static uint32_t rng(){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
int main(){
  for(int i=0;i<64;i++)mockDigital[i]=HIGH;
  Motors motor;motor.begin();
  for(int trial=0;trial<2200;trial++){
    Settings s=defaultSettings();setProfile(s,uint8_t(rng()%5));
    s.finishOn=uint8_t(rng()%2);s.navMode=uint8_t(rng()%3);
    Controller c;c.start(0,0);
    uint32_t t=0;
    for(int i=0;i<100;i++){
      t+=3;
      SensorFrame f={};
      const unsigned mode=rng()%26;
      f.pos=int16_t(int(rng()%13001)-6500);
      f.seen=(mode!=0 && mode!=1);
      f.active=f.seen?uint8_t(1+rng()%12):0;
      f.center=f.seen && f.pos>-1800 && f.pos<1800;
      f.leftEdge=f.seen && f.pos<-3900;
      f.rightEdge=f.seen && f.pos>3900;
      f.junction=mode==3;
      f.fullMarker=mode==4;
      f.confidence=f.seen?uint16_t(300+rng()%701):0;
      Command out=c.tick(f,s,t,.003f);
      assert(out.left>=-int(s.maxPWM) && out.left<=int(s.maxPWM));
      assert(out.right>=-int(s.maxPWM) && out.right<=int(s.maxPWM));
      // No reverse wheel in ordinary FOLLOW in the conservative default mode.
#if !LFR_ALLOW_FOLLOW_REVERSE
      if(c.state()==DriveState::Follow){assert(out.left>=0 && out.right>=0);}
#endif
      if(!c.active()){assert(out.left==0 && out.right==0);break;}
      motor.drive(out,s,true,.003f);
      assert(motor.left()>=-int(s.maxPWM) && motor.left()<=int(s.maxPWM));
      assert(motor.right()>=-int(s.maxPWM) && motor.right()<=int(s.maxPWM));
      assert(mockPwm[PB8]>=0 && mockPwm[PB8]<=int(s.maxPWM));
      assert(mockPwm[PB9]>=0 && mockPwm[PB9]<=int(s.maxPWM));
      assert(mockPwm[PA0]>=0 && mockPwm[PA0]<=int(s.maxPWM));
      assert(mockPwm[PA1]>=0 && mockPwm[PA1]<=int(s.maxPWM));
      assert(!(mockPwm[PB8]>0 && mockPwm[PB9]>0));
      assert(!(mockPwm[PA0]>0 && mockPwm[PA1]>0));
    }
    motor.stop();
  }
  puts("PASS 2200 randomized controller traces; motor PWM bounds, mutual-exclusion and state invariants");
}
