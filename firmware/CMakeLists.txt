include(${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake)

cmake_minimum_required(VERSION 3.16)

enable_language(ASM C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

################################################################################
## Project Configuration
################################################################################

project(d0-reader)

add_executable(
    d0-reader
    source/ethernetif.c
    source/gpio.cpp
    source/main.cpp
    source/uart.cpp
    freertos/list.c
    freertos/queue.c
    freertos/tasks.c
    freertos/portable/GCC/ARM_CM3/port.c
    freertos/portable/MemMang/heap_1.c
    lwip/contrib/ports/freertos/sys_arch.c
    lwip/src/apps/mdns/mdns_domain.c
    lwip/src/apps/mdns/mdns_out.c
    lwip/src/apps/mdns/mdns.c
    lwip/src/api/api_lib.c
    lwip/src/api/api_msg.c
    lwip/src/api/err.c
    lwip/src/api/netbuf.c
    lwip/src/api/sockets.c
    lwip/src/api/tcpip.c
    lwip/src/core/def.c
    lwip/src/core/inet_chksum.c
    lwip/src/core/init.c
    lwip/src/core/ip.c
    lwip/src/core/ipv4/acd.c
    lwip/src/core/ipv4/autoip.c
    lwip/src/core/ipv4/dhcp.c
    lwip/src/core/ipv4/etharp.c
    lwip/src/core/ipv4/icmp.c
    lwip/src/core/ipv4/igmp.c
    lwip/src/core/ipv4/ip4_addr.c
    lwip/src/core/ipv4/ip4_frag.c
    lwip/src/core/ipv4/ip4.c
    lwip/src/core/mem.c
    lwip/src/core/memp.c
    lwip/src/core/netif.c
    lwip/src/core/pbuf.c
    lwip/src/core/raw.c
    lwip/src/core/stats.c
    lwip/src/core/sys.c
    lwip/src/core/tcp_in.c
    lwip/src/core/tcp_out.c
    lwip/src/core/tcp.c
    lwip/src/core/timeouts.c
    lwip/src/core/udp.c
    lwip/src/netif/ethernet.c
    stm32f1xx_hal_driver/Src/stm32f1xx_hal_cortex.c
    stm32f1xx_hal_driver/Src/stm32f1xx_hal_eth.c
    stm32f1xx_hal_driver/Src/stm32f1xx_hal_gpio.c
    stm32f1xx_hal_driver/Src/stm32f1xx_hal_rcc.c
    stm32f1xx_hal_driver/Src/stm32f1xx_hal.c
    cmsis_device_f1/Source/Templates/gcc/startup_stm32f107xc.s
)

target_include_directories(
    d0-reader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_SOURCE_DIR}/source/config
    ${CMAKE_CURRENT_SOURCE_DIR}/freertos/include
    ${CMAKE_CURRENT_SOURCE_DIR}/freertos/portable/GCC/ARM_CM3
    ${CMAKE_CURRENT_SOURCE_DIR}/lwip/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lwip/contrib/ports/freertos/include
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx_hal_driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/cmsis_device_f1/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/cmsis_core/Include
)

target_compile_definitions(
    d0-reader PUBLIC
    STM32F107xC
    $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>
)

target_compile_options(
    d0-reader PUBLIC
    -mcpu=cortex-m3 -mthumb
)

target_link_options(
    d0-reader PUBLIC
    -mcpu=cortex-m3 -mthumb
    -specs=rdimon.specs
    -T${CMAKE_CURRENT_SOURCE_DIR}/memory.ld
)

set_target_properties(
    d0-reader PROPERTIES
    INTERFACE_LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/memory.ld
)

add_custom_target(
    createhex ALL
    DEPENDS d0-reader
    COMMAND ${CMAKE_OBJCOPY} -O ihex d0-reader.elf d0-reader.hex
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
