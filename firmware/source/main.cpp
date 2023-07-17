#include <cstdint>
#include "digitalout.h"
#include "stm32h7xx_ll_rcc.h"

int main(void) {
    // Enable GPIO clocks
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;
    //__IO uint32_t tmpreg = READ_BIT(RCC->AHB4ENR, RCC_AHB4ENR_GPIOBEN);

    DigitalOut dbg0(GPIOB, LL_GPIO_PIN_14, false);
    DigitalOut dbg1(GPIOE, LL_GPIO_PIN_1, true);

    while(1);

    return 0;
}

volatile const std::uintptr_t g_interrupt_table[]
__attribute__((section(".interrupt_table"))) {
    reinterpret_cast<std::uintptr_t>(nullptr),
};
