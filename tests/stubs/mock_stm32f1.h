#pragma once
// Host model of the few STM32F1 registers used by the direct sensor path
// (GPIOA->BSRR MUX select, ADC1 single conversion, RCC clocks). Force-include
// it together with -DSTM32F1xx; bit values match the STM32F103 reference manual.
#include <cstdint>
#include "Arduino.h"

#define RCC_APB2ENR_ADC1EN   (1U<<9)
#define RCC_CFGR_ADCPRE      (3U<<14)
#define RCC_CFGR_ADCPRE_DIV6 (2U<<14)
#define ADC_CR2_ADON    (1U<<0)
#define ADC_CR2_CAL     (1U<<2)
#define ADC_CR2_RSTCAL  (1U<<3)
#define ADC_CR2_EXTSEL  (7U<<17)
#define ADC_CR2_EXTTRIG (1U<<20)
#define ADC_CR2_SWSTART (1U<<22)
#define ADC_SR_EOC      (1U<<1)

struct MockRcc {uint32_t APB2ENR=0,CFGR=0;};

// BSRR write: low half sets PAx, high half resets PAx (set wins), applied to
// mockDigital[x] so the existing MUX/ADC model sees the selected channel.
struct MockBsrr {
  uint32_t writes=0;
  MockBsrr &operator=(uint32_t v) {
    for(uint8_t pin=0;pin<16;++pin) {
      if(v&(1U<<(pin+16))) mockDigital[pin]=LOW;
      if(v&(1U<<pin)) mockDigital[pin]=HIGH;
    }
    ++writes;return *this;
  }
};
struct MockGpio {MockBsrr BSRR;};

// CR2: RSTCAL/CAL complete immediately; SWSTART converts the selected channel.
struct MockAdcCr2 {
  uint32_t v=0;
  operator uint32_t() const {return v;}
  MockAdcCr2 &operator=(uint32_t x) {v=x;return *this;}
  MockAdcCr2 &operator|=(uint32_t x);
};
// Reading DR clears EOC, as on the real ADC.
struct MockAdcDr {
  uint32_t v=0;
  operator uint32_t() const;
};
struct MockAdc {
  uint32_t CR1=0,SR=0,SMPR2=0,SQR1=0,SQR3=0;
  MockAdcCr2 CR2;
  MockAdcDr DR;
  bool dead=false;  // never completes a conversion (fault-path test)
  int conversions=0,resetCalibrations=0,calibrations=0;
};
inline MockAdc mockAdc1Instance;
inline MockRcc mockRccInstance;
inline MockGpio mockGpioAInstance;

inline MockAdcCr2 &MockAdcCr2::operator|=(uint32_t x) {
  MockAdc &adc=mockAdc1Instance;
  v|=x;
  if(x&ADC_CR2_RSTCAL) {++adc.resetCalibrations;v&=~ADC_CR2_RSTCAL;}
  if(x&ADC_CR2_CAL) {++adc.calibrations;v&=~ADC_CR2_CAL;}
  if(x&ADC_CR2_SWSTART) {
    v&=~ADC_CR2_SWSTART;
    const bool swTrigger=(v&ADC_CR2_EXTTRIG) && (v&ADC_CR2_EXTSEL)==ADC_CR2_EXTSEL;
    if(!adc.dead && (v&ADC_CR2_ADON) && swTrigger) {
      const int mux=mockDigital[PA7] | mockDigital[PA6]<<1 |
                    mockDigital[PA4]<<2 | mockDigital[PA3]<<3;
      // Only the configured channel (PA2 = IN2) carries the MUX output.
      adc.DR.v=(adc.SQR3==2U)?uint32_t(mockAdcHook?mockAdcHook(mux):mockMuxADC[mux]):0U;
      adc.SR|=ADC_SR_EOC;
      ++adc.conversions;
    }
  }
  return *this;
}
inline MockAdcDr::operator uint32_t() const {
  mockAdc1Instance.SR&=~ADC_SR_EOC;
  return v;
}

#define ADC1  (&mockAdc1Instance)
#define RCC   (&mockRccInstance)
#define GPIOA (&mockGpioAInstance)
