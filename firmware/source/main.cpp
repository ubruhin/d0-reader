#include "gpio.h"
#include "uart.h"
#include "ethernetif.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lwip/apps/mdns.h"
#include "lwip/dhcp.h"
#include "lwip/igmp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "lwip/timeouts.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#include "stm32f1xx_hal.h"
#include <cassert>
#include <cstring>

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

  // Set flash wait states to 1 and enable prefetch buffer.
  FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
  FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_1;
  FLASH->ACR |= (uint32_t)FLASH_ACR_PRFTBE;

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

static volatile bool tcpInitDone = false;
static void tcpInitDoneCallback(void* arg) {
  UNUSED(arg);
  tcpInitDone = true;
}

static void mainTask(void* params) {
    UNUSED(params);

  // Configure priority grouping for FreeRTOS.
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  // Enable peripheral clocks.
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();

  // Initialize general purpose I/Os.
  //Gpio led = Gpio::outputPushPull(GPIOB, 14U, false);

  // Initialize UART.
  __HAL_RCC_USART3_CLK_ENABLE();
  __HAL_AFIO_REMAP_USART3_PARTIAL();
  Gpio::alternatePushPull(GPIOC, 10U, true);
  Gpio::inputFloating(GPIOC, 11U);
  Uart uart(USART3, USART3_IRQn, SystemCoreClock);
  uart.setBaudrate(19200U);
  uart.enable();

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

  tcpip_init(NULL, NULL);

  //while(!tcpInitDone) {
  //  vTaskDelay(1);
  //}

  // Initialize TCP/IP stack.
  struct netif gnetif;
  netif_add_noaddr(&gnetif, NULL, &ethernetif_init, &ethernet_input);
  netif_set_default(&gnetif);
  netif_set_link_callback(&gnetif, ethernetif_update_config);

  // Setup mDNS responses.
  mdns_resp_init();
  mdns_resp_add_netif(&gnetif, gnetif.hostname);
  mdns_resp_add_service(&gnetif, "SmartMeter", "_smartmeter", DNSSD_PROTO_TCP, 80, NULL, NULL);

  // Start application.
  Gpio led = Gpio::outputPushPull(GPIOB, 14U, true);

  /*char rec_buffer[10];
	char* rec_data;
	uint8_t rec_size;
	uint8_t client_disconnect = 1;
	int32_t res;
	const uint16_t server_port = 42;

	int32_t tcp_server_socket = 0;
	int32_t tcp_client_socket = 0;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	tcp_server_socket = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = lwip_htonl(INADDR_ANY);
	server_addr.sin_port = htons(server_port);
	res = lwip_bind(tcp_server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

	res = lwip_listen(tcp_server_socket, 5);

	//Up to here there are no errors
	while(1){
		//Accept new client
		tcp_client_socket = lwip_accept(tcp_server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

		//After the third call of accept() it returns the error -1
		if (tcp_client_socket < 0){
      led.toggle();
		} else {
      led.setLow();
      client_disconnect = 1;
      while(client_disconnect){
        //receive messages from the client
        rec_size = lwip_recv(tcp_client_socket, rec_buffer, 128, 0);
        if (rec_size == 0) { //client disconnected from server
          client_disconnect = 0; //go back to accept()
          lwip_close(tcp_client_socket);
        }
      }
      led.setHigh();
    }
    vTaskDelay(300);

	}*/


  uint32_t dhcpFineTimer = 0;
  while (1) {
    //if (netif_is_link_up(&gnetif)) {
    //  led.setHigh();
    //} else {
    //  led.setLow();
    //}

    ethernetif_input(&gnetif);
    sys_check_timeouts();

    if (HAL_GetTick() - dhcpFineTimer >= DHCP_FINE_TIMER_MSECS) {
      dhcpFineTimer = HAL_GetTick();
      ethernetif_update_link_status(&gnetif);
      DHCP_Process(&gnetif);
      led.toggle();

      //const char* msg = "Hello world!\n";
      //socket.write((const uint8_t*)msg, strlen(msg));
      //uart.sendBlocking((const uint8_t*)"Hello!\n", 8U);
    }
  }
}

int main() {
  TaskHandle_t taskHandle;
  xTaskCreate(mainTask, "mainTask", 512, NULL, 0, &taskHandle);
  assert(taskHandle);
  vTaskStartScheduler();
  while(1);
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

extern "C" void HAL_Delay(uint32_t Delay) {
  vTaskDelay(Delay);
}

extern "C" void xPortSysTickHandler(void);

extern "C" void SysTick_Handler(void) {
  HAL_IncTick();
  xPortSysTickHandler();
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
  UNUSED(xTask);
  UNUSED(pcTaskName);
  __disable_irq();
  while (1);
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line) {
  UNUSED(file);
  UNUSED(line);
  __disable_irq();
  while (1);
}
#endif

#ifndef NDEBUG
void __assert_func(const char* file, int line, const char* func, const char* cond) {
  UNUSED(file);
  UNUSED(line);
  UNUSED(func);
  UNUSED(cond);
  __disable_irq();
  while (1);
}
#endif
