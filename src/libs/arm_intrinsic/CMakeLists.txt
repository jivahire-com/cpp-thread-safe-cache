# Copyright (c) Microsoft Corporation. All rights reserved.


set(TARGET arm_intrinsic)

if("$ENV{REPO_APP_TOOLCHAIN}" STREQUAL "i386-pc-windows-msvc")
    add_library(${TARGET} STATIC win32/arm_intrinsic.c)
    target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/win32/inc)
elseif("$ENV{REPO_APP_TOOLCHAIN}" STREQUAL "arm-eabi-aarch")
    add_library(${TARGET} STATIC cortex_m7/arm_intrinsic.c)
    target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cortex_m7/inc)
endif()

lint_target(${TARGET})
format_target(${TARGET})

add_library(ms::lib::arm_intrinsic ALIAS ${TARGET})
