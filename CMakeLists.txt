#
# Copyright (c) Microsoft Corporation. All rights reserved.
#

cmake_minimum_required(VERSION 3.24)

project(kingsgate.mscp VERSION $ENV{MajorVersion}.$ENV{MinorVersion}.$ENV{PatchVersion}.0 LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_OBJECT_PATH_MAX 512) # set Max path length on CMake to be higher then default for Windows build

# Base directories
set(REPO_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

# Steer SCP/MCP builds to use stdint data types when ingesting common headers shared with UEFI
add_compile_definitions(SCP_CONFIG_VARIABLE_TYPE)

# Suppress rtosmon log prints
add_compile_definitions(SUPPRESS_RTOSMON_LOG_PRINT)

# Verbosity levels for INFO, DEBUG and CRITICAL below
# SILIBS_PRINT_VERBOSITY_LEVEL_NONE     0x0
# SILIBS_PRINT_VERBOSITY_LEVEL_CRITICAL 0x1
# SILIBS_PRINT_VERBOSITY_LEVEL_INFO     0x3
# SILIBS_PRINT_VERBOSITY_LEVEL_DEBUG    0x7
# SILIBS_PRINT_VERBOSITY_LEVEL_ALL      0xF
add_compile_definitions(SILIBS_PRINT_VERBOSITY_LEVEL=0x1) # Set to Critical

add_compile_definitions(FPFW_DEBUG)

# Suppress strncpy() is deprecated warnings in Unit test windows setup.
# strncpy() is used in Event trace
if("$ENV{REPO_APP_TOOLCHAIN}" STREQUAL "i386-pc-windows-msvc")
    add_definitions( -D_CRT_SECURE_NO_WARNINGS)
endif()

# Python3 executable path
set (Python3_EXECUTABLE $ENV{REPO_APP_PATH_python.win64}/tools/python.exe)

# Build type defines
add_compile_options(
  "$<$<CONFIG:RELEASE>:-DNDEBUG>"
  "$<$<CONFIG:DEBUG>:-DDEBUG>"
)

# Toolchain scripts
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/lint.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/format.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/coverage.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/mock.cmake)

# Repo Scripts
include(${REPO_ROOT_DIR}/tools/cmakes/scripts/unit_test.cmake)

# Build sources
add_subdirectory(src)
add_subdirectory(tests)

enabled_on(arm-eabi-aarch)

# Define a custom target that depends on all other targets
add_custom_target(all_app_targets ALL
    DEPENDS mcp_fw scp_fw mcp_boot_loader scp_boot_loader
)

# create file if all of the targets built
add_custom_command(
    TARGET all_app_targets
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_BINARY_DIR}/.allbuilt"
)
