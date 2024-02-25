#pragma once

#include "stm32f1xx.h"

class Uart {
public:
  Uart() = delete;
  Uart(const Uart& other) = delete;
  Uart(USART_TypeDef* uart, IRQn_Type irq, uint32_t clkFreq);

  void enable();
  void disable();
  void setBaudrate(uint32_t baud);
  void sendBlocking(const uint8_t* data, uint32_t len);
  void startReceiver();
  void stopReceiver();

private:

private:
  USART_TypeDef* mUart;
  const IRQn_Type mIrq;
  const uint32_t mClkFreq;
};
