#include "gpio.h"

Gpio::Gpio(GPIO_TypeDef* gpio, uint32_t pinNumber)
  : mGpio(gpio), mPinMask(1U << pinNumber) {
}

void Gpio::setHigh() {
  mGpio->BSRR = mPinMask;
}

void Gpio::setLow() {
  mGpio->BSRR = mPinMask << 16U;
}

void Gpio::toggle() {
  if (mGpio->ODR & mPinMask) {
    setLow();
  } else {
    setHigh();
  }
}

Gpio Gpio::inputFloating(GPIO_TypeDef* gpio, uint32_t pinNumber) {
  Gpio obj(gpio, pinNumber);
  setCrl(gpio, pinNumber, 0b00U, 0b01U);
  return obj;
}

Gpio Gpio::outputPushPull(GPIO_TypeDef* gpio, uint32_t pinNumber, bool state) {
  Gpio obj(gpio, pinNumber);
  if (state) {
    obj.setHigh();
  } else {
    obj.setLow();
  }
  setCrl(gpio, pinNumber, 0b10U, 0b00U);
  return obj;
}

Gpio Gpio::alternatePushPull(GPIO_TypeDef* gpio, uint32_t pinNumber) {
  Gpio obj(gpio, pinNumber);
  setCrl(gpio, pinNumber, 0b11U, 0b10U);
  return obj;
}

void Gpio::setCrl(GPIO_TypeDef* gpio, uint32_t pin, uint32_t mode, uint32_t cnf) {
  const uint32_t mask = 0xFU << ((pin % 8U) * 4U);
  const uint32_t value = (mode | (cnf << 2U)) << ((pin % 8U) * 4U);
  if (pin >= 8U) {
    gpio->CRH = (gpio->CRH & ~mask) | value;
  } else {
    gpio->CRL = (gpio->CRL & ~mask) | value;
  }
}
