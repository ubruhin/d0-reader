#pragma once

#include <freertos/FreeRTOS.h>
#include <hal/uart_types.h>
#include <soc/gpio_num.h>

#include <cstdint>

namespace app {

/**
 * @brief Driver for UART peripheral
 */
class Uart {
public:
  Uart() = delete;
  Uart(const Uart& other) = delete;
  Uart& operator=(const Uart& rhs) = delete;

  /**
   * @brief Construct a new Uart object
   *
   * @param uart  UART peripheral
   * @param rxPin RXD GPIO pin number
   * @param txPin TXD GPIO pin number
   */
  explicit Uart(uart_port_t uart, int rxPin, int txPin);

  /**
   * @brief Set the baudrate
   *
   * @param baud  Baudrate [bps]
   */
  void setBaudrate(int baud);

  /**
   * @brief Read received data
   *
   * @param buf   Buffer
   * @param size  Size of buffer
   * @return int  Number or received bytes
   */
  int read(char* buf, int size);

  /**
   * @brief Write data
   */
  void writeBlocking(const char* data);

private:
  const uart_port_t mUart;
  QueueHandle_t mQueue;
};

}  // namespace app
