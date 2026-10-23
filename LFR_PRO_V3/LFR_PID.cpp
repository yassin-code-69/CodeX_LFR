#include "LFR_PID.h"
#include <math.h>
namespace lfr {
static float bound(float x,float lower,float upper) {
  return x<lower?lower:(x>upper?upper:x);
}
void PID::reset(float initialError) {
  xEst_=initialError; vEst_=0.0f;
  lastError_=initialError; dFiltered_=0.0f; integral_=0.0f;
  initialized_=true;
}
float PID::update(float error, float dt, const Settings &cfg,
                  float limit, bool integrate,
                  float kpScale, float kdScale) {
  if (!(dt>0.0f) || dt>0.03f) {
    reset(error); return 0.0f;
  }
  if (!initialized_) reset(error);
  const float kp=float(cfg.kpX10)*0.1f * kpScale;
  const float ki=float(cfg.kiX100)*0.01f;
  const float kd=float(cfg.kdX100)*0.01f * kdScale;
#if LFR_PID_USE_ALPHA_BETA
  // Experimental observer retained for A/B comparisons only. A full
  // covariance-based Kalman filter is NOT implemented here.
  const float xPred=xEst_+vEst_*dt;
  const float residual=error-xPred;
  const float absResidual=fabsf(residual);
  const float alpha=(absResidual>0.6f)?0.50f:0.36f;
  const float beta=(absResidual>0.6f)?0.09f:0.05f;
  xEst_=xPred+alpha*residual;
  vEst_=bound(vEst_+(beta/dt)*residual,-180.0f,180.0f);
  dFiltered_=0.60f*dFiltered_+0.40f*vEst_;
#else
  // Default: bounded measured derivative with a time-constant low-pass.
  // Rate and damping remain comparable if the loop varies around 333 Hz.
  const float jump=error-lastError_;
  const float maxRate=fabsf(jump)>2.8f?90.0f:180.0f;
  const float measuredRate=bound(jump/dt,-maxRate,maxRate);
  const float filterAlpha=dt/(0.009f+dt);
  dFiltered_+=(measuredRate-dFiltered_)*filterAlpha;
#endif
  lastError_=error;
  // Anti-windup: integrate only near the line and while output is not saturated.
  // Decay stored integral during corners / branch decisions / large errors.
  // Dual-zone precision response:
  // Center zone (|error| < 0.35): silky smooth straight tracking with zero nervous twitch.
  // Edge zone (|error| >= 0.35): razor-sharp progressive grip with cubic exponential torque.
  const float errorAbs=fabsf(error);
  float shapedError=error;
  if(errorAbs<0.35f) {
    const float ratio=errorAbs/0.35f;
    shapedError=error*(0.85f+0.15f*ratio);
  }
  const float errorCubed=shapedError*shapedError*shapedError;
  const float pTerm=kp*(shapedError+0.006f*errorCubed);
  const float pd=pTerm+kd*dFiltered_;
  if (integrate && fabsf(error)<2.8f && fabsf(pd+ki*integral_)<limit*0.80f) {
    integral_=bound(integral_+error*dt,-5.0f,5.0f);
  } else {
    integral_*=0.965f;
  }
  return bound(pd+ki*integral_,-limit,limit);
}
}
