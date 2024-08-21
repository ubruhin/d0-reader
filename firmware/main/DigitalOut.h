#pragma once

#include <soc/gpio_num.h>

#include <cstdint>

namespace app {

/**
 * @brief Driver for the a simple digital output
 */
class DigitalOut {
public:
  DigitalOut() = delete;
  DigitalOut(const DigitalOut& other) = delete;
  DigitalOut& operator=(const DigitalOut& rhs) = delete;

  /**
   * @brief Construct a new DigitalOut object
   *
   * @param pin GPIO pin number
   */
  explicit DigitalOut(gpio_num_t pin);

  /**
   * @brief Set output to high
   */
  void setHigh();

  /**
   * @brief Set output to low
   */
  void setLow();

private:
  const gpio_num_t mPin;
};

}  // namespace app
