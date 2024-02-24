#pragma once

#include "stm32f1xx.h"

class Gpio {
public:
  Gpio() = delete;
  Gpio(const Gpio& other) : mGpio(other.mGpio), mPinMask(other.mPinMask) {}

  void setHigh();
  void setLow();
  void toggle();

  static Gpio inputFloating(GPIO_TypeDef* gpio, uint32_t pinNumber);
  static Gpio outputPushPull(GPIO_TypeDef* gpio, uint32_t pinNumber, bool state);
  static Gpio alternatePushPull(GPIO_TypeDef* gpio, uint32_t pinNumber);

private:
  Gpio(GPIO_TypeDef* gpio, uint32_t pinNumber);
  static void setCrl(GPIO_TypeDef* gpio, uint32_t pin, uint32_t mode, uint32_t cnf);

private:
  GPIO_TypeDef* mGpio;
  const uint32_t mPinMask;
};
