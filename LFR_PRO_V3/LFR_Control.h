#pragma once
#include "LFR_Config.h"
#include "LFR_PID.h"
namespace lfr {
class Controller {
public:
  void start(uint32_t nowMs,int16_t initialPosition,bool startingOnFullMarker=false);
  void stop();
  Command tick(const SensorFrame &f,const Settings &s,uint32_t nowMs,float dt);
  DriveState state() const {return state_;}
  bool active() const;
  uint8_t junctionCount() const {return junctions_;}
  // SCRIPT steps whose manoeuvre completed (line reacquired). A step being
  // executed is pending, not used: a failed turn never consumes a step.
  uint8_t routeUsed() const {return routeUsed_;}
  bool routePending() const {return routePending_;}
  const char* status() const;
  // V3.5: CLASSIC (default) or FAST turn phases. RAM only, set before start().
  void setTurnMode(TurnMode m) {turnMode_=m;}
  TurnMode turnMode() const {return turnMode_;}
  Command lastCommand() const {return lastCmd_;}
private:
  DriveState state_=DriveState::Idle;
  PID pid_;
  uint32_t started_=0,entered_=0,cooldownUntil_=0,clearSince_=0;
  int16_t lastPos_=0;
  int8_t pivotDirection_=-1;
  uint8_t seenFrames_=0,centerFrames_=0,edgeFrames_=0,junctionFrames_=0,markerFrames_=0,straightFrames_=0;
  uint8_t reacquireFrames_=0,stableApproachFrames_=0;
  int8_t edgeDirection_=0;
  bool oldCenterCleared_=false,junctionLatched_=false;
  bool startPad_=false,finishEligible_=false;
  uint32_t lastJunctionAt_=0;
  uint16_t pivotMinimumMs_=90,pivotMaximumMs_=900;
  RouteStep pending_=RouteStep::Straight;
  uint8_t junctions_=0,routeUsed_=0;
  // V3.4 route transaction and event context.
  bool routePending_=false;     // a SCRIPT step is being executed
  uint8_t commitFrames_=0;      // clean frames after a STRAIGHT step
  bool markerArmed_=false;      // this marker may be a crossing (T/+)
  uint32_t lastCleanMs_=0;      // last clean centred frame (approach history)
  int8_t fanSide_=0;            // side branch candidate: -1 left, +1 right
  uint32_t fanSince_=0;
  uint8_t fanArms_=0;           // arms seen during the candidate: bit0 left, bit1 right
  uint8_t junctionArms_=0;      // arms seen during junction frames: +bit2 centre
  int8_t cornerSide_=0;         // side of the most recent connected fan
  uint32_t cornerSeenMs_=0;
  // Corner Anticipation: counts consecutive frames where outer sensors detect line approaching edges
  uint8_t cornerBrakeFrames_=0;
  // Ghost Recovery Memory: remembers last good line state for intelligent gap recovery
  float lastGoodError_=0.0f;
  float lastGoodRate_=0.0f;
  // V3.5 FAST turn state. cmdAvg_ is the smoothed COMMANDED forward PWM, a
  // speed proxy only (no encoders): it is not a measured wheel speed.
  TurnMode turnMode_=TurnMode::Classic;
  Command lastCmd_={0,0};
  float cmdAvg_=0.0f;
  uint32_t now_=0,exitRampStart_=0;
  bool exitRamp_=false;
  int8_t brakeSide_=0;
  uint16_t brakeMs_=0,brakePivotMax_=900;
  void enter(DriveState state,uint32_t nowMs);
  Command decide(const SensorFrame &f,const Settings &s,uint32_t nowMs,float dt);
  Command beginCorner(int8_t side,uint32_t nowMs,uint16_t maximum,const Settings &s);
  void startPivot(int8_t direction,uint32_t nowMs,bool wasCenter,
                  uint16_t minimum=90,uint16_t maximum=900);
  Command pivot(const Settings &s) const;
  bool choose(const Settings &s,const SensorFrame &f,RouteStep &choice);
  void commitRoute();
  Command takeJunction(const SensorFrame &options,const SensorFrame &f,
                       const Settings &s,uint32_t nowMs,bool strict);
  Command follow(const SensorFrame &f,const Settings &s,float dt);
};
}
