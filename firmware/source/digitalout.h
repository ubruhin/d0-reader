#pragma once

#include "stm32h7xx_ll_gpio.h"

class DigitalOut {
public:
  DigitalOut()                        = delete;
  DigitalOut(const DigitalOut& other) = delete;
  DigitalOut(GPIO_TypeDef* gpio, uint32_t pin, bool state);
  DigitalOut(GPIO_TypeDef* gpio, uint32_t pin, bool state,
             uint32_t alternateFunction);

  void setHigh();
  void setLow();
  void toggle();

private:
  GPIO_TypeDef* mGpio;
  uint32_t      mPinMask;
};
