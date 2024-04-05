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

# Set C flags
set(CMAKE_C_FLAGS "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb -nostartfiles -L${CMAKE_CURRENT_LIST_DIR}/ld -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb -nostartfiles -L${CMAKE_CURRENT_LIST_DIR}/ld -Wall -Wextra -Werror")
set(CMAKE_ASM_FLAGS "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb -nostartfiles -L${CMAKE_CURRENT_LIST_DIR}/ld")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-mcpu=${CMAKE_SYSTEM_PROCESSOR} -mthumb -L${CMAKE_CURRENT_LIST_DIR}/ld")

set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

# Enable features
option(REPO_ENABLE_TESTS "Compiles unit tests" ON)
option(REPO_ENABLE_COVERAGE "Enables code coverage in compilation" ON)

# Set repo utilities
set(REPO_CLANG_TIDY "${CMAKE_SOURCE_DIR}/tools/cmakes/toolchain/arm-eabi-aarch/clang-tidy.cmd" 
    "--quiet" 
    "--extra-arg=--target=thumb")

set(REPO_IWYU "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmakes/toolchain/arm-eabi-aarch/iwyu.cmd" 
    "-Xiwyu" "--quoted_includes_first" 
    "-Xiwyu" "--mapping_file=${CMAKE_SOURCE_DIR}/.iwyu")

set(REPO_CLANG_FORMAT "$ENV{REPO_APP_PATH_llvm.win64}/bin/clang-format.exe")

# Utilities
set(OBJ_COPY "$ENV{REPO_APP_PATH_gcc.arm.eabi.aarch-win64}/bin/arm-none-eabi-objcopy.exe")

file(GLOB COMMON_LINK_FILES ${CMAKE_CURRENT_LIST_DIR}/ld/*.ld)
define_property(TARGET PROPERTY FIRMWARE_BIN 
    BRIEF_DOCS "Binary file of the output firmware" 
    FULL_DOCS "Binary file of the output firmware")
