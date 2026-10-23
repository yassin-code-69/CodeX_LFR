#include "LFR_Control.h"
#include <math.h>
namespace lfr {
static float clampF(float v,float a,float b) {
  return v<a?a:(v>b?b:v);
}
void Controller::enter(DriveState state,uint32_t nowMs) {
  state_=state;entered_=nowMs;
  seenFrames_=0;centerFrames_=0;edgeFrames_=0;markerFrames_=0;
  straightFrames_=0;junctionFrames_=0;reacquireFrames_=0;
}
void Controller::start(uint32_t nowMs,int16_t initialPosition,bool startingOnFullMarker) {
  enter(DriveState::Follow,nowMs);
  started_=nowMs;
  cooldownUntil_=0;clearSince_=0;
  lastPos_=initialPosition;
  stableApproachFrames_=0;
  edgeDirection_=0;oldCenterCleared_=false;junctionLatched_=false;
  startPad_=startingOnFullMarker;finishEligible_=false;lastJunctionAt_=0;
  pivotDirection_=-1;pivotMinimumMs_=38;pivotMaximumMs_=900;
  junctions_=routeUsed_=0;
  routePending_=false;commitFrames_=0;markerArmed_=false;
  lastCleanMs_=0;fanSide_=0;fanSince_=0;fanArms_=0;cornerSide_=0;cornerSeenMs_=0;
  pending_=RouteStep::Straight;
  cornerBrakeFrames_=0;
  lastGoodError_=0.0f;lastGoodRate_=0.0f;
  pid_.reset(float(initialPosition)*0.001f);
}
void Controller::stop() {state_=DriveState::Idle;startPad_=false;pid_.reset();}
bool Controller::active() const {
  return state_==DriveState::Follow || state_==DriveState::Gap ||
         state_==DriveState::Pivot || state_==DriveState::JunctionAdvance ||
         state_==DriveState::Marker;
}
const char* Controller::status() const {
  switch(state_) {
    case DriveState::Idle:return "IDLE";
    case DriveState::Follow:return "FOLLOW";
    case DriveState::Gap:return "GAP";
    case DriveState::Pivot:return "PIVOT";
    case DriveState::JunctionAdvance:return "JUNCTION";
    case DriveState::Marker:return "MARKER";
    case DriveState::Finished:return "FINISHED";
    default:return "FAULT";
  }
}
void Controller::startPivot(int8_t direction,uint32_t nowMs,bool wasCenter,
                            uint16_t minimum,uint16_t maximum) {
  pivotDirection_=direction<0?-1:1;
  pivotMinimumMs_=minimum;pivotMaximumMs_=maximum;
  oldCenterCleared_=!wasCenter;
  pid_.reset(float(lastPos_)*0.001f);
  enter(DriveState::Pivot,nowMs);
}
Command Controller::pivot(const Settings &s) const {
  const int16_t pwm=s.turnPWM;
  return pivotDirection_<0?Command{int16_t(-pwm),pwm}:Command{pwm,int16_t(-pwm)};
}
bool Controller::choose(const Settings &s,const SensorFrame &f,RouteStep &choice) {
  if(s.navMode==uint8_t(NavMode::Script)) {
    // Propose only: the step is committed after the line is reacquired.
    if(routeUsed_>=s.routeCount) return false;
    choice=RouteStep(s.route[routeUsed_]);routePending_=true;return true;
  }
  if(s.navMode==uint8_t(NavMode::Left))
    choice=f.leftEdge?RouteStep::Left:(f.center?RouteStep::Straight:RouteStep::Right);
  else if(s.navMode==uint8_t(NavMode::Right))
    choice=f.rightEdge?RouteStep::Right:(f.center?RouteStep::Straight:RouteStep::Left);
  else
    choice=f.center?RouteStep::Straight:(f.leftEdge?RouteStep::Left:RouteStep::Right);
  return true;
}
void Controller::commitRoute() {
  if(routePending_) {++routeUsed_;routePending_=false;}
  commitFrames_=0;
}
// One junction decision. `options` says which branches physically exist
// (leftEdge/rightEdge/center). Strict callers stop on a SCRIPT step that asks
// for a branch the pattern did not show instead of turning into nothing.
Command Controller::takeJunction(const SensorFrame &options,const SensorFrame &f,
                                 const Settings &s,uint32_t nowMs,bool strict) {
  const Command stop={0,0};
  RouteStep chosen;
  if(!choose(s,options,chosen)) {enter(DriveState::Fault,nowMs);return stop;}
  if(strict && ((chosen==RouteStep::Straight && !options.center) ||
                (chosen==RouteStep::Left && !options.leftEdge) ||
                (chosen==RouteStep::Right && !options.rightEdge))) {
    enter(DriveState::Fault,nowMs);return stop;
  }
  junctionLatched_=true;clearSince_=0;
  ++junctions_;
  lastJunctionAt_=nowMs;
  stableApproachFrames_=0;fanSide_=0;
  cooldownUntil_=nowMs+360;
  pending_=chosen;
  lastPos_=f.pos;
  enter(DriveState::JunctionAdvance,nowMs);
  return Command{int16_t(s.curvePWM),int16_t(s.curvePWM)};
}
Command Controller::follow(const SensorFrame &f,const Settings &s,float dt) {
  const float error=float(f.pos)*0.001f;
  const float previous=float(lastPos_)*0.001f;
  const float rate=clampF((error-previous)/dt,-180.f,180.f);
  const float anglePenalty=fabsf(error)/3.8f;
  const float speedPenalty=fabsf(rate)/70.f;
  // Radar Edge Lookahead: Outer sensors (0-1 or 12-13) detect line approaching edge,
  // pre-shaping curvature before centrifugal drift throws the chassis off!
  float edgePenalty=0.0f;
  if((f.leftEdge || f.rightEdge) && !f.junction && !f.fullMarker && fabsf(error)>3.2f) {
    edgePenalty=0.80f;
  }

  // ── Corner Anticipation & Feedforward Pre-braking ──
  // When outer sensors (positions 2-3 or 10-11) start picking up the line,
  // it means a curve is approaching. Pre-decelerate BEFORE PID reacts.
  const bool outerLeft =(f.norm[2]>=280 || f.norm[3]>=280) && fabsf(error)>1.5f;
  const bool outerRight=(f.norm[10]>=280 || f.norm[11]>=280) && fabsf(error)>1.5f;
  if((outerLeft || outerRight) && !f.junction && !f.fullMarker) {
    if(cornerBrakeFrames_<20) ++cornerBrakeFrames_;
  } else {
    if(cornerBrakeFrames_>0) --cornerBrakeFrames_; // gradual release, not abrupt
  }
  // Feedforward brake intensity scales with how long we've been seeing approach
  const float anticipationPenalty=clampF(float(cornerBrakeFrames_)/8.0f,0.0f,0.75f);
  const float curvature=clampF(fmaxf(fmaxf(fmaxf(anglePenalty,speedPenalty),edgePenalty),anticipationPenalty),0.f,1.f);

  // Dynamic Straightaway Turbo Boost:
  // Smoothly ramp up toward maxPWM when dead center (<0.65) and calm rate (<18)
  if(fabsf(error)<0.65f && fabsf(rate)<18.0f && f.confidence>=350 &&
     f.active<=4 && f.center && !f.junction && !f.fullMarker) {
    if(straightFrames_<40) ++straightFrames_;
  } else {
    straightFrames_=0;
  }
  float boostSpeed=float(s.basePWM);
  if(straightFrames_>=15) {
    const float boostFraction=float(straightFrames_-15)/25.0f; // gentle 0.0 to 1.0 ramp
    boostSpeed+=float(s.maxPWM-s.basePWM)*(boostFraction>1.0f?1.0f:boostFraction);
  }

  // Smooth continuous deceleration with curvature (no discrete step shocks)
  float speed=boostSpeed-(float(boostSpeed-s.curvePWM)*curvature);
  if(f.confidence<300) speed-=6.f;
  if(speed<float(s.curvePWM)) speed=s.curvePWM;

  // ── Adaptive Gain Scheduling ──
  // 1. High-Speed Straight Sprint (straightFrames_ >= 10):
  //    As the robot accelerates along open straights, reduce Kp (down to 0.82x) to eliminate
  //    high-speed tail flutter, and increase Kd (up to 1.28x) for rock-solid straight damping.
  // 2. Corner Entry & Tight Curves (curvature > 0.28):
  //    As lateral error or approach rate rises, scale up Kp (up to 1.20x) to give maximum
  //    steering torque and bite, while maintaining balanced Kd.
  float kpScale = 1.0f;
  float kdScale = 1.0f;
  if(straightFrames_ >= 10) {
    const float straightFactor = clampF(float(straightFrames_ - 10) / 20.0f, 0.0f, 1.0f);
    kpScale -= 0.18f * straightFactor;
    kdScale += 0.28f * straightFactor;
  } else if(curvature > 0.28f) {
    const float curveFactor = clampF((curvature - 0.28f) / 0.50f, 0.0f, 1.0f);
    kpScale += 0.20f * curveFactor;
    kdScale += 0.10f * curveFactor;
  }

  const bool stable = f.active<=5 && f.confidence>=250 && !f.junction &&
                      !f.fullMarker && fabsf(error)<2.8f;
  const float steering=pid_.update(error,dt,s,float(s.maxPWM),stable,kpScale,kdScale);
  lastPos_=f.pos;

  // ── Ghost Recovery Memory: save last good line state ──
  if(f.seen && !f.junction && !f.fullMarker && f.confidence>=350) {
    lastGoodError_=error;
    lastGoodRate_=rate;
  }

  // Default: no reverse wheel command while FOLLOWING. For a 510-530g robot,
  // grip/brake response must be measured before enabling reverse counter-torque.
#if LFR_ALLOW_FOLLOW_REVERSE
  const int16_t minWheel = -int16_t(s.turnPWM * 0.65f);
#else
  const int16_t minWheel = 0;
#endif
  const int16_t left=clamp16(int32_t(speed+steering),minWheel,s.maxPWM);
  const int16_t right=clamp16(int32_t(speed-steering),minWheel,s.maxPWM);
  return Command{left,right};
}
Command Controller::tick(const SensorFrame &f,const Settings &s,
                         uint32_t nowMs,float dt) {
  const Command stop={0,0};
  if(!active()) return stop;
  if(!(dt>0.f) || dt>0.030f) {enter(DriveState::Fault,nowMs);return stop;}
  const uint32_t elapsed=nowMs-entered_;
  // A black START pad has no unique centroid. Clear it before arming FINISH.
  if(startPad_) {
    if(elapsed>LFR_START_PAD_TIMEOUT_MS) {enter(DriveState::Fault,nowMs);return stop;}
    // In WHITE line mode a black pad is "no line" (fullBlack, !seen).
    // Broad frames while leaving it at an angle are still the pad.
    if(f.fullMarker || (f.fullBlack && !f.seen) || (f.seen && f.active>5))
      return Command{int16_t(s.curvePWM),int16_t(s.curvePWM)};
    startPad_=false;
    enter(DriveState::Follow,nowMs);
    pid_.reset(float(f.pos)*0.001f);lastPos_=f.pos;
    // Leaving the pad is not a junction (it must not consume a SCRIPT step).
    junctionLatched_=true;clearSince_=nowMs;cooldownUntil_=nowMs+180;
    if(!f.seen) {enter(DriveState::Gap,nowMs);return stop;}
    return Command{int16_t(s.curvePWM),int16_t(s.curvePWM)};
  }
  // Clean centered approach is one supporting cue for an explicit finish pad.
  // A short non-clean glitch (< LFR_APPROACH_HOLD_MS) keeps the history.
  if(state_==DriveState::Follow && f.seen && f.center && !f.junction &&
     !f.fullMarker && f.active<=4 && f.pos>-1300 && f.pos<1300) {
    if(stableApproachFrames_<30) ++stableApproachFrames_;
    lastCleanMs_=nowMs;
  } else if(!f.fullMarker && (nowMs-lastCleanMs_)>LFR_APPROACH_HOLD_MS) {
    stableApproachFrames_=0;
  }
  if(f.seen && !f.fullMarker && !f.junction) {
    if(seenFrames_<8)++seenFrames_;
  } else seenFrames_=0;

  if(state_==DriveState::Pivot) {
    if(elapsed>pivotMaximumMs_) {enter(DriveState::Fault,nowMs);return stop;}
    if(!f.center) oldCenterCleared_=true;
    if(oldCenterCleared_ && f.seen && f.center && f.active<=5 &&
       !f.junction && !f.fullMarker && f.pos>-1900 && f.pos<1900) {
      if(centerFrames_<8)++centerFrames_;
    } else centerFrames_=0;
    if(elapsed>=pivotMinimumMs_ && centerFrames_>=2) {
      commitRoute();  // a SCRIPT turn counts only once the line is back
      enter(DriveState::Follow,nowMs);
      pid_.reset(float(f.pos)*0.001f);lastPos_=f.pos;
      cooldownUntil_=nowMs+150;
      return follow(f,s,dt);
    }
    return pivot(s);
  }

  if(state_==DriveState::Gap) {
    // A line can reappear as a junction or full-black patch. Do not insist on
    // f.junction==false for reacquisition. Require two consecutive seen frames.
    if(f.seen) {
      if(reacquireFrames_<3) ++reacquireFrames_;
    } else reacquireFrames_=0;
    if(reacquireFrames_>=2) {
      if(f.fullMarker) {
        finishEligible_=false;markerArmed_=false;
        enter(DriveState::Marker,nowMs);
        return Command{int16_t(s.curvePWM),int16_t(s.curvePWM)};
      }
      enter(DriveState::Follow,nowMs);
      pid_.reset(float(f.pos)*0.001f);lastPos_=f.pos;
      cooldownUntil_=f.junction?nowMs:nowMs+150;
      return Command{int16_t(s.curvePWM),int16_t(s.curvePWM)};
    }
    // Bounded blind crossing: LFR_GAP_RECOVER_MS in Recover, LFR_GAP_STOP_MS in Stop
    if(elapsed>(s.gapMode==uint8_t(GapMode::Stop)?LFR_GAP_STOP_MS:LFR_GAP_RECOVER_MS)) {
      enter(DriveState::Fault,nowMs);return stop;
    }
    // ── Ghost Recovery: Curve-Aware Predictive Arc Coasting ──
    // Instead of driving blind or with a rigid tiny offset, coast along the circular arc
    // calculated from line position offset and lateral yaw velocity right before the gap.
    const int16_t forwardPWM = s.curvePWM; // no high-speed blind coasting
    if(s.gapMode==uint8_t(GapMode::Stop))return stop;
    // Arc decay over elapsed time (decays by up to 35% across the gap to prevent over-steering)
    const float decay = 1.0f - clampF(float(elapsed) / 220.0f, 0.0f, 0.35f);
    // Predictive differential: combines position displacement + angular rate
    const float arcBias = (lastGoodError_ * 10.0f + lastGoodRate_ * 0.12f) * decay;
    const int16_t steerPWM = clamp16(int32_t(arcBias), -42, 42);
    return Command{clamp16(int32_t(forwardPWM) + steerPWM, 0, s.maxPWM),
                   clamp16(int32_t(forwardPWM) - steerPWM, 0, s.maxPWM)};
  }

  if(state_==DriveState::Marker) {
    // Full black may mean a pad, thick crossing, or FINISH. It cannot be
    // uniquely identified from the same 14 reflectance samples. FINISH is
    // OFF by default and must only be enabled after track-rule validation.
    if(f.fullMarker) {
      if(markerFrames_<250) ++markerFrames_;
      if(finishEligible_ && markerFrames_>=3 && elapsed>=LFR_FINISH_CONFIRM_MS) {
        enter(DriveState::Finished,nowMs);return stop;
      }
      if(elapsed>LFR_MARKER_TIMEOUT_MS) {
        enter(DriveState::Fault,nowMs);return stop; // never drive blind indefinitely
      }
    } else {
      // Still on a broad dark area (e.g. leaving it at an angle): not an exit yet.
      if(f.seen && f.active>5) {
        if(elapsed>LFR_MARKER_TIMEOUT_MS) {enter(DriveState::Fault,nowMs);return stop;}
        return Command{int16_t(s.curvePWM),int16_t(s.curvePWM)};
      }
      // A thin marker is not automatically a finish without sufficient width.
      if(finishEligible_ && markerFrames_>=3 && elapsed>=LFR_FINISH_CONFIRM_MS) {
        enter(DriveState::Finished,nowMs);return stop;
      }
      // Cleared quickly: it was a crossing line (T or +), not a pad. The whole
      // bar was dark, so both arms exist; straight exists only if the line
      // continues. Same optics as a narrow pad: LFR_CROSSING_MAX_MS decides.
      if(markerArmed_ && elapsed<=LFR_CROSSING_MAX_MS) {
        SensorFrame options=f;
        options.leftEdge=true;options.rightEdge=true;
        options.center=f.seen;  // the continuing line may be off-centre
        pid_.reset(float(f.pos)*0.001f);
        return takeJunction(options,f,s,nowMs,true);
      }
      enter(DriveState::Follow,nowMs);
      pid_.reset(float(f.pos)*0.001f);lastPos_=f.pos;
      cooldownUntil_=nowMs+180;junctionLatched_=true;clearSince_=nowMs;
      if(!f.seen) {enter(DriveState::Gap,nowMs);return stop;}
    }
    return Command{int16_t(s.curvePWM),int16_t(s.curvePWM)};
  }

  if(state_==DriveState::JunctionAdvance) {
    if(elapsed>=s.junctionAdvanceMs) {
      if(pending_==RouteStep::Straight) {
        enter(DriveState::Follow,nowMs);
        pid_.reset(float(f.pos)*0.001f);
        return Command{int16_t(s.curvePWM),int16_t(s.curvePWM)};
      }
      const bool isU=pending_==RouteStep::UTurn;
      const int8_t side=pending_==RouteStep::Right?1:-1;
      startPivot(side,nowMs,f.center,isU?285:90,isU?1450:900);
      return pivot(s);
    }
    return Command{int16_t(s.curvePWM),int16_t(s.curvePWM)};
  }

  if(state_!=DriveState::Follow) return stop;
  if(f.fullMarker) {
    finishEligible_=s.finishOn && (nowMs-started_)>2500U &&
       (s.navMode!=uint8_t(NavMode::Script) || routeUsed_==s.routeCount) &&
       !routePending_ && stableApproachFrames_>=5 && !junctionLatched_ &&
       (lastJunctionAt_==0 || (nowMs-lastJunctionAt_)>450U);
    // Only a fresh dark patch may be a crossing; not one we just turned at.
    markerArmed_=!junctionLatched_ && nowMs>=cooldownUntil_ && !routePending_;
    fanSide_=0;
    enter(DriveState::Marker,nowMs);
    return Command{int16_t(s.curvePWM),int16_t(s.curvePWM)};
  }
  if(!f.seen) {
    // Immediate edge loss: steer toward last observed edge rather than coast ahead.
    // A connected fan just before the loss means a 90-degree corner on that side
    // even though the blob centroid (lastPos_) never reached the edge.
    const bool cornerFan=cornerSide_!=0 && (nowMs-cornerSeenMs_)<=LFR_FAN_CONFIRM_MS;
    fanSide_=0;
    if(cornerFan || lastPos_<=-3800 || lastPos_>=3800) {
      startPivot(cornerFan?cornerSide_:(lastPos_<0?-1:1),nowMs,false,38,800);
      cornerSide_=0;
      return pivot(s);
    }
    pid_.reset(float(lastPos_)*0.001f);
    enter(DriveState::Gap,nowMs);
    const int16_t forwardPWM = s.gapMode==uint8_t(GapMode::Stop)?0:s.curvePWM;
    return Command{forwardPWM,forwardPWM};
  }

  // ── Enhanced 90° Sharp Turn Fast Lock ──
  // Detect when the line has moved to the extreme edge (only far-side sensors lit).
  // Responds in 1 frame with strong position guard for instant zero-radius pivot.
  int8_t side=0;
  if(!f.junction && !f.center && f.active<=5 &&
     f.leftEdge && !f.rightEdge && f.pos<-4200) side=-1;
  else if(!f.junction && !f.center && f.active<=5 &&
          f.rightEdge && !f.leftEdge && f.pos>4200) side=1;
  if(side==0) {edgeFrames_=0;edgeDirection_=0;}
  else if(side!=edgeDirection_) {edgeDirection_=side;edgeFrames_=1;}
  else if(edgeFrames_<5) ++edgeFrames_;
  // Single-frame lock when position is extreme (>4800) for immediate response
  const bool instantLock=(side!=0 && fabsf(float(f.pos))>4800.0f);
  if(edgeFrames_>=2 || instantLock) {
    lastPos_=f.pos;
    startPivot(side,nowMs,f.center,38,900);
    return pivot(s);
  }

  // Connected side branch: one blob from an edge into the centre. A 90-degree
  // corner shows the same blob, so decide only after it passes: the centred line
  // continues -> branch (junction); the centre is lost -> corner (pivot above).
  const bool fan=f.center && !f.junction && f.groups==1 &&
                 f.active>=LFR_FAN_MIN_ACTIVE && (f.leftEdge!=f.rightEdge);
  if(fanSide_!=0) fanArms_|=uint8_t((f.leftEdge?1:0)|(f.rightEdge?2:0));
  if(fan) {
    cornerSide_=f.leftEdge?-1:1;cornerSeenMs_=nowMs;
    if(fanSide_==0 && stableApproachFrames_>=5 && !junctionLatched_ &&
       nowMs>=cooldownUntil_ && !routePending_) {
      fanSide_=cornerSide_;fanSince_=nowMs;
      fanArms_=uint8_t(fanSide_<0?1:2);
    }
  } else if(fanSide_!=0) {
    const bool resumed=f.center && !f.leftEdge && !f.rightEdge &&
                       !f.junction && f.active<=5;
    if(resumed) {
      SensorFrame options=f;
      // An angled + shows one arm, then the other: offer every arm seen.
      options.center=true;options.leftEdge=(fanArms_&1)!=0;options.rightEdge=(fanArms_&2)!=0;
      cornerSide_=0;
      return takeJunction(options,f,s,nowMs,true);
    }
    if(!f.center || (nowMs-fanSince_)>LFR_FAN_CONFIRM_MS) fanSide_=0;
  }

  // A SCRIPT "straight" commits after a few clean centred frames past the junction.
  if(routePending_ && pending_==RouteStep::Straight) {
    const bool clean=!f.junction && f.active<=5;  // narrow line anywhere: junction passed
    commitFrames_=clean?uint8_t(commitFrames_<8?commitFrames_+1:8):0;
    if(commitFrames_>=3) commitRoute();
  }

  // Junction counter debounced and latched until the marker fully clears.
  if(f.junction) {
    clearSince_=0;
    if(junctionFrames_<8) ++junctionFrames_;
  } else {
    junctionFrames_=0;
    if(clearSince_==0) clearSince_=nowMs;
    if(junctionLatched_ && (nowMs-clearSince_)>=175 && nowMs>=cooldownUntil_)
      junctionLatched_=false;
  }
  if(f.junction && junctionFrames_>=3 && !junctionLatched_ &&
     nowMs>=cooldownUntil_ && !routePending_) {
    return takeJunction(f,f,s,nowMs,false);
  }
  return follow(f,s,dt);
}
}
