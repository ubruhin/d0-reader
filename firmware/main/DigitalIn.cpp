#include "DigitalIn.h"

#include <driver/gpio.h>
#include <esp_err.h>

namespace app {

DigitalIn::DigitalIn(gpio_num_t pin) : mPin(pin) {
  gpio_config_t gpioCfg = {
      .pin_bit_mask = 1ULL << pin,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&gpioCfg);
}

bool DigitalIn::read() const {
  ESP_ERROR_CHECK(gpio_set_level(mPin, 1U));
  return gpio_get_level(mPin) != 0;
}

}  // namespace app
