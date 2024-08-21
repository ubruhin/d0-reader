#include "DigitalIn.h"
#include "DigitalOut.h"
#include "Uart.h"

#include <esp_eth.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>

#include <cassert>

namespace app {

static const char* TAG = "main";

static void ethEventHandler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data) {
  uint8_t mac[6] = {0};
  esp_eth_handle_t eth = *(esp_eth_handle_t*)event_data;

  switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
      esp_eth_ioctl(eth, ETH_CMD_G_MAC_ADDR, mac);
      ESP_LOGI(TAG, "Ethernet Link Up");
      ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x", mac[0],
               mac[1], mac[2], mac[3], mac[4], mac[5]);
      break;
    case ETHERNET_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "Ethernet Link Down");
      break;
    case ETHERNET_EVENT_START:
      ESP_LOGI(TAG, "Ethernet Started");
      break;
    case ETHERNET_EVENT_STOP:
      ESP_LOGI(TAG, "Ethernet Stopped");
      break;
    default:
      break;
  }
}

static void gotIpHandler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data) {
  ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
  const esp_netif_ip_info_t* ip_info = &event->ip_info;

  ESP_LOGI(TAG, "Ethernet Got IP Address");
  ESP_LOGI(TAG, "~~~~~~~~~~~");
  ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
  ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
  ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
  ESP_LOGI(TAG, "~~~~~~~~~~~");
}

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "Booting application...");

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_netif_init());

  DigitalOut pwrLed(GPIO_NUM_7);
  pwrLed.setHigh();

  DigitalOut testPoint1(GPIO_NUM_8);

  DigitalIn button(GPIO_NUM_38);

  Uart irUart(UART_NUM_1, 39, 5);

  eth_esp32_emac_config_t cfgEmac = ETH_ESP32_EMAC_DEFAULT_CONFIG();
  cfgEmac.smi_mdc_gpio_num = GPIO_NUM_32;
  cfgEmac.smi_mdio_gpio_num = GPIO_NUM_33;
  cfgEmac.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
  eth_mac_config_t cfgMac = ETH_MAC_DEFAULT_CONFIG();
  esp_eth_mac_t* mac = esp_eth_mac_new_esp32(&cfgEmac, &cfgMac);
  assert(mac);

  eth_phy_config_t cfgPhy = ETH_PHY_DEFAULT_CONFIG();
  cfgPhy.phy_addr = 0;
  cfgPhy.reset_gpio_num = GPIO_NUM_4;
  esp_eth_phy_t* phy = esp_eth_phy_new_lan87xx(&cfgPhy);
  assert(phy);

  esp_eth_config_t cfgEth = ETH_DEFAULT_CONFIG(mac, phy);
  esp_eth_handle_t ethHandle = NULL;
  ESP_ERROR_CHECK(esp_eth_driver_install(&cfgEth, &ethHandle));

  esp_netif_config_t cfgNetif = ESP_NETIF_DEFAULT_ETH();
  esp_netif_t* ethNetif = esp_netif_new(&cfgNetif);
  ESP_ERROR_CHECK(
      esp_netif_attach(ethNetif, esp_eth_new_netif_glue(ethHandle)));
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                             &ethEventHandler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                                             &gotIpHandler, NULL));
  ESP_ERROR_CHECK(esp_eth_start(ethHandle));

  while (1);
}

}  // namespace app
