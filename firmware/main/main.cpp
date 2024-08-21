#include "Atm90e26.h"
#include "Buzzer.h"
#include "FreeRTOSConfig.h"
#include "Led.h"
#include "MeasureThread.h"
#include "OcppConnection.h"
#include "Relay.h"
#include "Webinterface.h"
#include "Wifi.h"

#include <MicroOcpp.h>
#include <MicroOcpp/Core/Configuration.h>
#include <driver/spi_common.h>
#include <esp_app_desc.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_spiffs.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <hal/spi_types.h>
#include <nvs_flash.h>

#include <cstring>

namespace app {

static const char* TAG = "main";

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "Booting application...");

  // Create event loop.
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Initialize NVS.
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize SPIFFS.
  esp_vfs_spiffs_conf_t spiffsCfg = {
      .base_path = "/spiffs",
      .partition_label = nullptr,
      .max_files = 5,
      .format_if_mount_failed = 1,
  };
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_spiffs_register(&spiffsCfg));

  // Initialize LED.
  Led led(22);
  led.set(0, 0, 255);

  // Initialize buzzer.
  Buzzer buzzer(23);

  // Initialize relay.
  Relay relay(GPIO_NUM_3);

  // Initialize SPI bus.
  spi_bus_config_t spiCfg = {
      .mosi_io_num = 2,
      .miso_io_num = 7,
      .sclk_io_num = 6,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .data4_io_num = -1,
      .data5_io_num = -1,
      .data6_io_num = -1,
      .data7_io_num = -1,
      .max_transfer_sz = 32,
      .flags = 0,
      .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
      .intr_flags = 0,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &spiCfg, SPI_DMA_CH_AUTO));

  // Initialize energy meter & start measure thread.
  Atm90e26 meter(SPI2_HOST, 18);
  MeasureThread measureThread(meter);

  // Initialize TCP/IP stack.
  ESP_ERROR_CHECK(esp_netif_init());

  // Initialize WiFi.
  Wifi wifi;

  // Initialize websocket connection.
  OcppConnection con;

  // Determine charger metadata.
  uint8_t mac[6] = {0};
  char serial[13];
  ESP_ERROR_CHECK(esp_base_mac_addr_get(mac));
  snprintf(serial, sizeof(serial), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5]);
  ChargerCredentials credentials("Sintio Plug Prototype", "Sintio",
                                 esp_app_get_description()->version, serial);

  // Initialize MicroOcpp.
  bool connectorPlugged = true;
  mocpp_initialize(con, credentials,
                   MicroOcpp::makeDefaultFilesystemAdapter(
                       MicroOcpp::FilesystemOpt::Deactivate));
  setOnResetExecute([](bool) { esp_restart(); });
  setConnectorPluggedInput([&]() { return connectorPlugged; });
  setEnergyMeterInput(
      [&measureThread]() { return measureThread.getForwardActiveEnergy(); });
  setPowerMeterInput(
      [&measureThread]() { return measureThread.getActivePower(); });
  addMeterValueInput([&measureThread]() { return measureThread.getCurrent(); },
                     "Current.Export", "A");
  addMeterValueInput([&measureThread]() { return measureThread.getVoltage(); },
                     "Voltage", "V");
  MicroOcpp::getConfigurationPublic("MeterValueSampleInterval")->setInt(10);
  MicroOcpp::getConfigurationPublic("MeterValuesSampledData")
      ->setString(
          "Energy.Active.Import.Register,"
          "Power.Active.Import,"
          "Current.Export,"
          "Voltage");

  // Start the webinterface.
  Webinterface web(wifi, con);

  // State.
  enum class State {
    Init,
    Disconnected,
    Connected,
    Charging
  } state = State::Init;
  TickType_t startChargeTime = 0;

  while (1) {
    vTaskDelay(10 / portTICK_PERIOD_MS);

    mocpp_loop();

    State newState = state;
    const TickType_t time = xTaskGetTickCount();
    if (ocppPermitsCharge()) {
      newState = State::Charging;
    } else if (con.isConnected()) {
      newState = State::Connected;
    } else {
      newState = State::Disconnected;
    }

    if ((newState == State::Charging) && (state != State::Charging)) {
      startChargeTime = time;
    }

    if (newState == State::Charging) {
      const int32_t dTime = (time - startChargeTime) % (2 * configTICK_RATE_HZ);
      if (dTime == 0) {
        led.set(0, 255, 0);
      } else if (dTime == configTICK_RATE_HZ) {
        led.clear();
      }
    } else if ((newState == State::Connected) && (newState != state)) {
      led.set(20, 50, 70);
    } else if ((newState == State::Disconnected) && (newState != state)) {
      led.set(255, 0, 0);
    }

    connectorPlugged = true;
    if ((newState == State::Charging) && (state != State::Charging)) {
      relay.switchOn();
      buzzer.beep(500, 700, 100, 200);
    } else if ((newState != State::Charging) && (state == State::Charging)) {
      relay.switchOff();
      buzzer.beep(700, 500, 100, 200);
      connectorPlugged = false;  // Workaround for Sintio App error.
    }

    state = newState;

    if (web.isRestartRequired()) {
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      esp_restart();
    }
  }
}

}  // namespace app
