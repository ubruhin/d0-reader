#include <esp_log.h>

namespace app {

static const char* TAG = "main";

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "Booting application...");
  while (1);
}

}  // namespace app
