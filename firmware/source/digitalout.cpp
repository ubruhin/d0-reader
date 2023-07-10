#include "digitalout.h"

DigitalOut::DigitalOut(GPIO_TypeDef* gpio, uint32_t pin, bool state)
  : mGpio(gpio), mPinMask(pin) {
  if (state) {
    setHigh();
  } else {
    setLow();
  }

  LL_GPIO_InitTypeDef params;
  LL_GPIO_StructInit(&params);
  params.Pin        = mPinMask;
  params.Mode       = LL_GPIO_MODE_OUTPUT;
  params.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  params.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  params.Pull       = LL_GPIO_PULL_NO;
  params.Alternate  = LL_GPIO_AF_0;
  LL_GPIO_Init(mGpio, &params);
}

DigitalOut::DigitalOut(GPIO_TypeDef* gpio, uint32_t pin, bool state,
                       uint32_t alternateFunction)
  : mGpio(gpio), mPinMask(pin) {
  if (state) {
    setHigh();
  } else {
    setLow();
  }

  LL_GPIO_InitTypeDef params;
  LL_GPIO_StructInit(&params);
  params.Pin        = mPinMask;
  params.Mode       = LL_GPIO_MODE_ALTERNATE;
  params.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  params.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  params.Pull       = LL_GPIO_PULL_NO;
  params.Alternate  = alternateFunction;
  LL_GPIO_Init(mGpio, &params);
}

void DigitalOut::setHigh() {
  LL_GPIO_SetOutputPin(mGpio, mPinMask);
}

void DigitalOut::setLow() {
  LL_GPIO_ResetOutputPin(mGpio, mPinMask);
}

void DigitalOut::toggle() {
  LL_GPIO_TogglePin(mGpio, mPinMask);
}
