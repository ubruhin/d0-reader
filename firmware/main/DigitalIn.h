#pragma once

#include <soc/gpio_num.h>

#include <cstdint>

namespace app {

/**
 * @brief Driver for the a simple digital input with pull-up
 */
class DigitalIn {
public:
  DigitalIn() = delete;
  DigitalIn(const DigitalIn& other) = delete;
  DigitalIn& operator=(const DigitalIn& rhs) = delete;

  /**
   * @brief Construct a new DigitalIn object
   *
   * @param pin GPIO pin number
   */
  explicit DigitalIn(gpio_num_t pin);

  /**
   * @brief Read input state
   */
  bool read() const;

private:
  const gpio_num_t mPin;
};

}  // namespace app
