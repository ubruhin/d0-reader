include(${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake)

cmake_minimum_required(VERSION 3.16)

################################################################################
## Build Configuration
################################################################################

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Wall -Wextra -Wpedantic -Wshadow -Wdouble-promotion
    -ffunction-sections -fdata-sections
    -fno-strict-aliasing -fno-builtin -fno-common
    -fno-exceptions -fno-rtti
    -g3
)

add_link_options(
    -Wl,--gc-sections,-print-memory-usage
)

set(CMAKE_C_FLAGS_DEBUG_INIT "-O0")
set(CMAKE_CXX_ASM_FLAGS_DEBUG_INIT "-O0")
set(CMAKE_ASM_FLAGS_DEBUG_INIT "")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT "")

set(CMAKE_C_FLAGS_RELEASE_INIT "-Os -flto")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Os -flto")
set(CMAKE_ASM_FLAGS_RELEASE_INIT "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT "-flto")

################################################################################
## Project Configuration
################################################################################

project(d0-reader)

add_definitions(
    -DSTM32H723xx
)

add_compile_options(
    -mcpu=cortex-m7 -mthumb
    -mfloat-abi=soft  #-mfloat-abi=hard -mfpu=fpv5-sp-d16
)

################################################################################
## CMSIS
################################################################################

add_library(cmsis INTERFACE)
target_include_directories(cmsis INTERFACE ${CMAKE_SOURCE_DIR}/cmsis)

################################################################################
## STM32H7xx
################################################################################

add_library(stm32h7xx INTERFACE)

target_include_directories(
    stm32h7xx INTERFACE
    ${CMAKE_SOURCE_DIR}/stm32h7xx
)

target_link_libraries(
    stm32h7xx INTERFACE
    cmsis
)

################################################################################
## STM32H7xx HAL
################################################################################

add_library(
    stm32h7xx_hal
    stm32h7xx_hal/stm32h7xx_ll_gpio.c
)

target_include_directories(
    stm32h7xx_hal PUBLIC
    ${CMAKE_SOURCE_DIR}/stm32h7xx_hal
)

target_compile_definitions(
    stm32h7xx_hal PUBLIC
    -DUSE_FULL_LL_DRIVER=1
)

target_link_libraries(
    stm32h7xx_hal PUBLIC
    cmsis
    stm32h7xx
)

################################################################################
## Executable
################################################################################

add_executable(
    d0-reader
    source/startup.cpp
    source/main.cpp
    source/digitalout.cpp
)

target_link_libraries(
    d0-reader PRIVATE
    cmsis
    stm32h7xx
    stm32h7xx_hal
)

target_link_options(
    d0-reader PUBLIC
    --specs=nano.specs
    -T${CMAKE_CURRENT_SOURCE_DIR}/stm32h723zg.ld
)

set_target_properties(
    d0-reader PROPERTIES
    SUFFIX ".bin"
    INTERFACE_LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/stm32h723zg.ld
)

add_custom_target(
    createhex ALL
    DEPENDS d0-reader
    COMMAND ${CMAKE_OBJCOPY} -O ihex d0-reader.bin d0-reader.hex
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
