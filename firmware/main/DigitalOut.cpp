#include "DigitalOut.h"

#include <driver/gpio.h>
#include <esp_err.h>

namespace app {

DigitalOut::DigitalOut(gpio_num_t pin) : mPin(pin) {
  gpio_config_t gpioCfg = {
      .pin_bit_mask = 1ULL << pin,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&gpioCfg);
  setLow();
}

void DigitalOut::setHigh() {
  ESP_ERROR_CHECK(gpio_set_level(mPin, 1U));
}

void DigitalOut::setLow() {
  ESP_ERROR_CHECK(gpio_set_level(mPin, 0U));
}

}  // namespace app
