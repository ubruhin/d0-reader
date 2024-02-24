#include "gpio.h"
#include "tcpsocket.h"
#include "ethernetif.h"
#include "lwip/apps/mdns.h"
#include "lwip/dhcp.h"
#include "lwip/igmp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "stm32f1xx_hal.h"
#include <cassert>

// System clock variable accessed by the HAL driver.
uint32_t SystemCoreClock = 36000000;

#define MAX_DHCP_TRIES  4

#define DHCP_OFF                   (uint8_t) 0
#define DHCP_START                 (uint8_t) 1
#define DHCP_WAIT_ADDRESS          (uint8_t) 2
#define DHCP_ADDRESS_ASSIGNED      (uint8_t) 3
#define DHCP_TIMEOUT               (uint8_t) 4
#define DHCP_LINK_DOWN             (uint8_t) 5
__IO uint8_t DHCP_state = DHCP_START;

static void DHCP_Process(struct netif *netif);

extern "C" void SystemInit(void) {
  // Reset all clocks.
  RCC->CR |= (uint32_t)0x00000001;
  RCC->CFGR &= (uint32_t)0xF0FF0000;
  RCC->CR &= (uint32_t)0xFEF6FFFF;
  RCC->CR &= (uint32_t)0xFFFBFFFF;
  RCC->CFGR &= (uint32_t)0xFF80FFFF;
  RCC->CR &= (uint32_t)0xEBFFFFFF;
  RCC->CIR = 0x00FF0000;
  RCC->CFGR2 = 0x00000000;

  // Configure vector table offset.
  SCB->VTOR = FLASH_BASE;

  // Set flash wait states to 1.
  FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
  FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_1;

  // Set clock dividers.
  RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;  // HCLK = SYSCLK
  RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1; // PCLK2 = HCLK
  RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1; // PCLK1 = HCLK

  // Enable PLL with PLLCLK = HSI / 2 * 9 = 36 MHz.
  RCC->CFGR &= (uint32_t)~(RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL);
  RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLXTPRE_PREDIV1 | RCC_CFGR_PLLMULL9);
  RCC->CR |= RCC_CR_PLLON;
  while((RCC->CR & RCC_CR_PLLRDY) == 0);

  // Select PLL as system clock source.
  RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
  RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08);
}

int main() {
  // Initialize SysTick to 1ms.
  HAL_Init();

  // Enable peripheral clocks.
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();

  // Initialize general purpose I/Os.
  Gpio led = Gpio::outputPushPull(GPIOB, 14U, false);

  // Initialize Ethernet hardware.
  Gpio::alternatePushPull(GPIOA, 2U);  // RMII_MDIO
  Gpio::alternatePushPull(GPIOB, 11U);  // RMII_TXEN
  Gpio::alternatePushPull(GPIOB, 12U);  // RMII_TXD0
  Gpio::alternatePushPull(GPIOB, 13U);  // RMII_TXD1
  Gpio::alternatePushPull(GPIOC, 1U);  // RMII_MDC
  Gpio::inputFloating(GPIOA, 1U);  // RMII_REFCLK
  Gpio::inputFloating(GPIOA, 7U);  // RMII_CRS_DV
  Gpio::inputFloating(GPIOC, 4U);  // RMII_RXD0
  Gpio::inputFloating(GPIOC, 5U);  // RMII_RXD1
  __HAL_AFIO_ETH_RMII();
  __HAL_RCC_ETHMAC_CLK_ENABLE();
  __HAL_RCC_ETHMACTX_CLK_ENABLE();
  __HAL_RCC_ETHMACRX_CLK_ENABLE();

  // Initialize TCP/IP stack.
  struct netif gnetif;
  lwip_init();
  netif_add_noaddr(&gnetif, NULL, &ethernetif_init, &ethernet_input);
  netif_set_default(&gnetif);
  netif_set_link_callback(&gnetif, ethernetif_update_config);

  // Setup mDNS responses.
  mdns_resp_init();
  mdns_resp_add_netif(&gnetif, gnetif.hostname);
  mdns_resp_add_service(&gnetif, "SmartMeter", "_smartmeter", DNSSD_PROTO_TCP, 80, NULL, NULL);

  // Start application.
  TcpSocket socket(42);

  uint32_t dhcpFineTimer = 0;
  while (1) {
    if (socket.canSend()) {
      led.setHigh();
    } else {
      led.setLow();
    }

    ethernetif_input(&gnetif);
    sys_check_timeouts();

    if (HAL_GetTick() - dhcpFineTimer >= DHCP_FINE_TIMER_MSECS) {
      dhcpFineTimer = HAL_GetTick();
      ethernetif_update_link_status(&gnetif);
      DHCP_Process(&gnetif);
    }
  }
}

void ethernetif_notify_conn_changed(struct netif *netif) {
  if(netif_is_link_up(netif)) {
    DHCP_state = DHCP_START;
    netif_set_up(netif);
  } else {
    DHCP_state = DHCP_LINK_DOWN;
    netif_set_down(netif);
  }
}

void DHCP_Process(struct netif *netif) {
  struct dhcp *dhcp;

  switch (DHCP_state) {
    case DHCP_START: {
      ip_addr_set_zero_ip4(&netif->ip_addr);
      ip_addr_set_zero_ip4(&netif->netmask);
      ip_addr_set_zero_ip4(&netif->gw);
      DHCP_state = DHCP_WAIT_ADDRESS;
      dhcp_start(netif);
      break;
    }

    case DHCP_WAIT_ADDRESS: {
      if (dhcp_supplied_address(netif)) {
        DHCP_state = DHCP_ADDRESS_ASSIGNED;
      } else {
        dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
        if (dhcp->tries > MAX_DHCP_TRIES) {
          DHCP_state = DHCP_TIMEOUT;
          dhcp_stop(netif);
        }
      }
      break;
    }

    case DHCP_LINK_DOWN: {
      dhcp_stop(netif);
      DHCP_state = DHCP_OFF;
      break;
    }

    default: {
      break;
    }
  }
}

extern "C" void SysTick_Handler(void) {
  HAL_IncTick();
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line) {
  UNUSED(file);
  UNUSED(line);
  while (1);
}
#endif

#ifdef DEBUG
void __assert_func(const char* file, int line, const char* func, const char* cond) {
  UNUSED(file);
  UNUSED(line);
  UNUSED(func);
  UNUSED(cond);
  while (1);
}
#endif
