#include "Uart.h"

#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_err.h>

#include <cstring>

namespace app {

Uart::Uart(uart_port_t uart, int rxPin, int txPin) : mUart(uart), mQueue() {
  uart_config_t cfg = {};
  cfg.baud_rate = 300;
  cfg.data_bits = UART_DATA_7_BITS;
  cfg.parity = UART_PARITY_EVEN;
  cfg.stop_bits = UART_STOP_BITS_1;
  cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  cfg.rx_flow_ctrl_thresh = 122;
  ESP_ERROR_CHECK(uart_param_config(uart, &cfg));
  ESP_ERROR_CHECK(
      uart_set_pin(uart, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  const int bufSize = 1024;
  ESP_ERROR_CHECK(uart_driver_install(uart, bufSize, bufSize, 10, &mQueue, 0));
}

void Uart::setBaudrate(int baud) {
  ESP_ERROR_CHECK(uart_set_baudrate(mUart, baud));
}

int Uart::read(char* buf, int size) {
  return uart_read_bytes(mUart, buf, size, 0);
}

void Uart::writeBlocking(const char* data) {
  uart_write_bytes(mUart, data, strlen(data));
  ESP_ERROR_CHECK(uart_wait_tx_done(mUart, portMAX_DELAY));
}

}  // namespace app
