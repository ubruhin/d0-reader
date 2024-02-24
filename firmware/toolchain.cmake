set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(TARGET_TRIPLET "arm-none-eabi-")

find_program(COMPILER_ON_PATH "${TARGET_TRIPLET}gcc${TOOLCHAIN_EXT}")

# Perform compiler test with the static library
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER    ${TARGET_TRIPLET}gcc${ARM_COMPILER_EXT})
set(CMAKE_CXX_COMPILER  ${TARGET_TRIPLET}g++${ARM_COMPILER_EXT})
set(CMAKE_ASM_COMPILER  ${TARGET_TRIPLET}gcc${ARM_COMPILER_EXT})
set(CMAKE_LINKER        ${TARGET_TRIPLET}gcc${ARM_COMPILER_EXT})
set(CMAKE_SIZE_UTIL     ${TARGET_TRIPLET}size${ARM_COMPILER_EXT})
set(CMAKE_OBJCOPY       ${TARGET_TRIPLET}objcopy${ARM_COMPILER_EXT})
set(CMAKE_OBJDUMP       ${TARGET_TRIPLET}objdump${ARM_COMPILER_EXT})
set(CMAKE_NM_UTIL       ${TARGET_TRIPLET}gcc-nm${ARM_COMPILER_EXT})
set(CMAKE_AR            ${TARGET_TRIPLET}gcc-ar${ARM_COMPILER_EXT})
set(CMAKE_RANLIB        ${TARGET_TRIPLET}gcc-ranlib${ARM_COMPILER_EXT})

set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

# default to Debug build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
        "Choose the type of build, options are: Debug Release." FORCE)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra -Wpedantic -Wshadow -Wdouble-promotion ")
string(APPEND CMAKE_C_FLAGS "-ffunction-sections -fdata-sections ")
string(APPEND CMAKE_C_FLAGS "-fno-strict-aliasing -fno-builtin -fno-common ")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ")
string(APPEND CMAKE_CXX_FLAGS "-fno-rtti -fno-exceptions ")
string(APPEND CMAKE_CXX_FLAGS "-fno-threadsafe-statics ")

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections ")
string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,-print-memory-usage ")
string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--no-warn-rwx-segment ")

set(CMAKE_ASM_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")

set(CMAKE_ASM_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_RELEASE "-Os -flto=auto")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -flto=auto")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto=auto")
