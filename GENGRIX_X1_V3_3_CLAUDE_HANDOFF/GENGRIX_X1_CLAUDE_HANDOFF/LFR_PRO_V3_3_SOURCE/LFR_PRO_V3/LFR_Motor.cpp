#include "LFR_Motor.h"
namespace lfr {
void Motors::begin() {
  pinMode(LFR_PIN_LEFT_A,OUTPUT); pinMode(LFR_PIN_LEFT_B,OUTPUT);
  pinMode(LFR_PIN_RIGHT_A,OUTPUT); pinMode(LFR_PIN_RIGHT_B,OUTPUT);
  digitalWrite(LFR_PIN_LEFT_A,LOW);digitalWrite(LFR_PIN_LEFT_B,LOW);
  digitalWrite(LFR_PIN_RIGHT_A,LOW);digitalWrite(LFR_PIN_RIGHT_B,LOW);
  analogWriteResolution(8);
  analogWriteFrequency(10000);
  stop();
}
void Motors::outputOne(uint32_t a,uint32_t b,int16_t pwm,uint8_t dutyCap) {
  if(pwm==0) {
    analogWrite(a,0);
    analogWrite(b,0);
    return;
  }
  // Deadband Compensation: map (0, 255] -> [DEADBAND, 255]
  // Overcomes gearbox static friction (stiction) for instantaneous micro-steering.
  const int16_t absPwm=abs(pwm);
  int16_t compensated=absPwm;
#if defined(LFR_MOTOR_DEADBAND) && (LFR_MOTOR_DEADBAND > 0)
  if(absPwm>0 && absPwm<255) {
    compensated=int16_t(LFR_MOTOR_DEADBAND +
                ((int32_t(absPwm) * (255 - LFR_MOTOR_DEADBAND)) / 255));
  }
#endif
  // A physical PWM duty MUST never exceed the selected maxPWM.
  if(compensated>dutyCap) compensated=dutyCap;
  if(pwm>0) {
    analogWrite(a,0);
    analogWrite(b,compensated); // positive = forward: V1 direction tested on robot
  } else {
    analogWrite(b,0);
    analogWrite(a,compensated);
  }
}
void Motors::stop() {
  left_=right_=0;
  outputOne(LFR_PIN_LEFT_A,LFR_PIN_LEFT_B,0,0);
  outputOne(LFR_PIN_RIGHT_A,LFR_PIN_RIGHT_B,0,0);
}
int16_t Motors::slew(int16_t current,int16_t target,const Settings &s,float dt) {
  // A reversal ALWAYS passes through 0; deceleration is faster than acceleration.
  if ((current>0 && target<0)||(current<0 && target>0)) target=0;
  const int16_t difference=target-current;
  // Settings encode logical PWM change per 5ms. Scale using observed dt so
  // 200Hz and 333Hz don't silently change braking strength by 67%.
  if(!(dt>0.0f) || dt>0.030f) dt=0.005f;
  const int16_t step5ms = (abs(target)<abs(current))?s.brakeStep:s.accelStep;
  int16_t step=int16_t(float(step5ms)*(dt/0.005f)+0.5f);
  if(step<1)step=1;
  if(difference>step) return current+step;
  if(difference<-step) return current-step;
  return target;
}
void Motors::drive(Command demand,const Settings &s,bool reverseAllowed,float dt) {
  const int16_t cap=s.maxPWM;
  if (!reverseAllowed) {
    if(demand.left<0)demand.left=0;
    if(demand.right<0)demand.right=0;
  }
  demand.left=clamp16(demand.left,-cap,cap);
  demand.right=clamp16(demand.right,-cap,cap);
  left_=slew(left_,demand.left,s,dt);
  right_=slew(right_,demand.right,s,dt);
  outputOne(LFR_PIN_LEFT_A,LFR_PIN_LEFT_B,left_,s.maxPWM);
  outputOne(LFR_PIN_RIGHT_A,LFR_PIN_RIGHT_B,right_,s.maxPWM);
}
}
