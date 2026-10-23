#pragma once
#include <Arduino.h>
#include "LFR_Config.h"
namespace lfr {
class Sensor {
public:
  void begin();
  void resetTracking();
  void capture(uint16_t destination[SENSOR_COUNT]);
  void read(SensorFrame &f,const Calibration &cal,bool valid,const Settings &s);
  // Precompute fixed-point 1/range multipliers after calibration changes.
  // Call this after every successful white/black calibration.
  void buildNormTable(const Calibration &cal);
  // Duration of the last read() in microseconds (scan + classification).
  uint16_t lastScanUs() const {return scanUs_;}
  // Latched when a direct ADC1 conversion never completes; do not race.
  bool adcFault() const {return adcFault_;}
private:
  uint16_t filtered_[SENSOR_COUNT]={};
  bool hasFilter_=false;
  int16_t lastPos_=0;
  bool hasPosition_=false;
  // Q16 fixed-point: normScale_[i] = (1000 << 16) / (black[i] - white[i])
  // Eliminates expensive per-frame integer division in the hot loop.
  uint32_t normScale_[SENSOR_COUNT]={};
  bool hasNormTable_=false;
  uint16_t scanUs_=0;
  bool adcFault_=false;
  void selectChannel(uint8_t channel);
  uint16_t adcChannel(uint8_t channel);
  uint16_t convert();
  void initFastADC();
};
}
