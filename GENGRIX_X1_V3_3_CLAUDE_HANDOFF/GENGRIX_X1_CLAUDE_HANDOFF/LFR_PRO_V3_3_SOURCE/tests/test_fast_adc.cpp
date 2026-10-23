#include <cassert>
#include <cstdio>
#include "Arduino.h"
#include "LFR_Config.h"
#include "LFR_Sensor.h"
using namespace lfr;
#ifndef STM32F1xx
#error "build with -DSTM32F1xx -include stubs/mock_stm32f1.h"
#endif
// Direct STM32F1 sensor path (LFR_FAST_SENSOR=1) against the register model.
static int channelValue(int mux) {return 120+mux*230;}  // distinct per MUX input
static int muxFromPins() {
  return mockDigital[PA7] | mockDigital[PA6]<<1 | mockDigital[PA4]<<2 | mockDigital[PA3]<<3;
}
static void testBsrrTable() {
  for(uint8_t ch=0;ch<16;++ch) {
    for(int p=0;p<16;++p) mockDigital[p]=((p+ch)&1)?HIGH:LOW;  // stale pin state
    GPIOA->BSRR=MUX_BSRR[ch];
    assert(muxFromPins()==ch);
    // Every MUX pin is either set or reset, and no other PAx pin is touched.
    assert(((MUX_BSRR[ch]&0xFFFFU)|(MUX_BSRR[ch]>>16))==MUX_PIN_MASK);
    assert(((MUX_BSRR[ch]&0xFFFFU)&(MUX_BSRR[ch]>>16))==0);
  }
  puts("PASS MUX BSRR table: all 16 selections drive exactly PA7/PA6/PA4/PA3");
}
static void testInit() {
  Sensor s;s.begin();
  assert(RCC->APB2ENR&RCC_APB2ENR_ADC1EN);
  assert((RCC->CFGR&RCC_CFGR_ADCPRE)==RCC_CFGR_ADCPRE_DIV6);
  const uint32_t cr2=ADC1->CR2;
  assert(cr2&ADC_CR2_ADON);
  assert((cr2&ADC_CR2_EXTSEL)==ADC_CR2_EXTSEL && (cr2&ADC_CR2_EXTTRIG));
  assert(((ADC1->SMPR2>>(LFR_ADC_CHANNEL*3U))&7U)==LFR_ADC_SAMPLE_CYCLES);
  assert(ADC1->CR1==0 && ADC1->SQR1==0 && ADC1->SQR3==LFR_ADC_CHANNEL);
  assert(mockAdc1Instance.resetCalibrations==1 && mockAdc1Instance.calibrations==1);
  puts("PASS direct ADC1 init: clock, /6 prescaler, SWSTART trigger, sample time, channel, calibration");
}
static void testScanOrderAndMirror() {
  for(int i=0;i<16;++i) mockMuxADC[i]=channelValue(i);
  Sensor s;s.begin();
  Settings st=defaultSettings();Calibration cal={};SensorFrame f={};
  const int readsBefore=mockAnalogReads;
  const int convBefore=mockAdc1Instance.conversions;
  const uint32_t bsrrBefore=mockGpioAInstance.BSRR.writes;
  s.read(f,cal,false,st);
  for(uint8_t i=0;i<SENSOR_COUNT;++i) assert(f.raw[i]==channelValue(MUX_BY_POSITION[i]));
  assert(mockAnalogReads==readsBefore);  // analogRead() is never used
  assert(mockAdc1Instance.conversions-convBefore==SENSOR_COUNT*(LFR_SENSOR_DUMMY_READ?2:1));
  assert(mockGpioAInstance.BSRR.writes-bsrrBefore==uint32_t(SENSOR_COUNT));
  assert(s.lastScanUs()==SENSOR_COUNT*LFR_MUX_SETTLE_US);  // host time = settle delays only
  st.mirror=1;s.resetTracking();s.read(f,cal,false,st);
  for(uint8_t i=0;i<SENSOR_COUNT;++i)
    assert(f.raw[i]==channelValue(MUX_BY_POSITION[SENSOR_COUNT-1-i]));
  assert(!s.adcFault());
  printf("PASS direct MUX+ADC1 scan: 14 channels in order, mirror, no analogRead (%d conversions/frame)\n",
         SENSOR_COUNT*(LFR_SENSOR_DUMMY_READ?2:1));
}
static void testDeadAdcFault() {
  Sensor s;s.begin();
  Settings st=defaultSettings();Calibration cal={};SensorFrame f={};
  mockAdc1Instance.dead=true;
  s.read(f,cal,false,st);
  mockAdc1Instance.dead=false;
  assert(s.adcFault());
  for(uint8_t i=0;i<SENSOR_COUNT;++i) assert(f.raw[i]==0);
  puts("PASS dead ADC1 conversion latches adcFault and reads zero");
}
int main() {
  for(int i=0;i<64;++i) mockDigital[i]=HIGH;
  testBsrrTable();testInit();testScanOrderAndMirror();testDeadAdcFault();
  puts("ALL DIRECT SENSOR PATH TESTS PASSED");
  return 0;
}
