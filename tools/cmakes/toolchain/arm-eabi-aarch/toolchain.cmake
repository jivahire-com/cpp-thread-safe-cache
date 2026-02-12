# Copyright (c) Microsoft Corporation. All rights reserved.

include($ENV{REPO_APP_ROOT}/tools/cmakes/scripts/enabled_on.cmake)
set_valid_enable_flags(arm-eabi-aarch)

# Set compiler
set(CMAKE_C_COMPILER_ID "GCC")
set(CMAKE_CXX_COMPILER_ID "GCC")
set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "cortex-m7")

set(CMAKE_C_COMPILER "$ENV{REPO_APP_PATH_gcc.arm.eabi.aarch-win64}/bin/arm-none-eabi-gcc.exe")
set(CMAKE_CXX_COMPILER "$ENV{REPO_APP_PATH_gcc.arm.eabi.aarch-win64}/bin/arm-none-eabi-g++.exe")
set(CMAKE_ASM_COMPILER "$ENV{REPO_APP_PATH_gcc.arm.eabi.aarch-win64}/bin/arm-none-eabi-gcc.exe")

# Enable Floating Point Unit (FPU)
option(ENABLE_FPU "Enable hardware floating-point unit for Cortex-M7" ON)
if(ENABLE_FPU)
    add_compile_definitions(__FPU_PRESENT=1)
    message(STATUS "Hardware Floating Point Unit (FPU) is enabled")
    string(APPEND CMAKE_C_FLAGS_INIT "-mfloat-abi=hard -mfpu=fpv5-sp-d16 ")
endif()

# Set C flags
set(CMAKE_C_FLAGS "-mcpu=${CMAKE_SYSTEM_PROCESSOR} ${CMAKE_C_FLAGS_INIT} -mthumb -nostartfiles -Wall -Wextra -Werror -ftest-coverage -ffunction-sections -fdata-sections -funwind-tables")
set(CMAKE_CXX_FLAGS "-mcpu=${CMAKE_SYSTEM_PROCESSOR} ${CMAKE_C_FLAGS_INIT} -mthumb -nostartfiles -Wall -Wextra -Werror -ftest-coverage -ffunction-sections -fdata-sections -funwind-tables")
set(CMAKE_ASM_FLAGS "-mcpu=${CMAKE_SYSTEM_PROCESSOR} ${CMAKE_C_FLAGS_INIT} -mthumb -nostartfiles -funwind-tables")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb -Wl,--gc-sections")

# Add Cortex m7 clock speed, ticks per second
SET(TX_CORE_CLOCK_SPEED "6250000")        # Default to 6.5MHz which is the clock speed on FPGA for mscp
SET(TX_CORE_TICKS_PER_SECOND "100")

set(__ICACHE_PRESENT 1)
set(__DCACHE_PRESENT 1)

set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

# Enable features
option(REPO_ENABLE_TESTS "Compiles unit tests" ON)
option(REPO_ENABLE_COVERAGE "Enables code coverage in compilation" ON)

# Enable response files
set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS TRUE)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS TRUE)
set(CMAKE_NINJA_FORCE_RESPONSE_FILE TRUE)

# Set repo utilities
set(REPO_CLANG_TIDY "${CMAKE_SOURCE_DIR}/tools/cmakes/toolchain/arm-eabi-aarch/clang-tidy.cmd")

# TODO: Enable IWYU once IWYU fails builds on warnings
# ADO: 1967581
#set(REPO_IWYU "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmakes/toolchain/arm-eabi-aarch/iwyu.cmd"
#    "-Xiwyu" "--quoted_includes_first"
#    "-Xiwyu" "--mapping_file=${CMAKE_SOURCE_DIR}/.iwyu")

set(REPO_CLANG_FORMAT "$ENV{REPO_APP_PATH_llvm.win64}/bin/clang-format.exe")

# Utilities
set(OBJ_COPY "$ENV{REPO_APP_PATH_gcc.arm.eabi.aarch-win64}/bin/arm-none-eabi-objcopy.exe")

file(GLOB COMMON_LINK_FILES ${CMAKE_CURRENT_LIST_DIR}/ld/*.ld)
define_property(TARGET PROPERTY FIRMWARE_BIN
    BRIEF_DOCS "Binary file of the output firmware"
    FULL_DOCS "Binary file of the output firmware")

# temporarily enable optimization for size
# revert:  https://azurecsi.visualstudio.com/Woodinville/_workitems/edit/1885631
add_compile_options(
    "-Os"
)