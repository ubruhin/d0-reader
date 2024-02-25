#include "uart.h"
#include "stm32f1xx_hal.h"

Uart::Uart(USART_TypeDef* uart, IRQn_Type irq, uint32_t clkFreq)
  : mUart(uart), mIrq(irq), mClkFreq(clkFreq) {
    mUart->CR1 |= USART_CR1_PCE;
    mUart->CR1 |= USART_CR1_TE;
}

void Uart::enable() {
  mUart->CR1 |= USART_CR1_UE;
}

void Uart::disable() {
  mUart->CR1 &= ~USART_CR1_UE;
}

void Uart::setBaudrate(uint32_t baud) {
  mUart->BRR = USART_BRR(mClkFreq, baud);
}

void Uart::sendBlocking(const uint8_t* data, uint32_t len) {
  for (uint32_t i = 0U; i < len; ++i) {
    mUart->DR = data[i];
    while (!(mUart->SR & USART_SR_TXE));
  }
}

void Uart::startReceiver() {
  mUart->CR1 |= USART_CR1_RE;
}

void Uart::stopReceiver() {
  mUart->CR1 &= ~USART_CR1_RE;
}
