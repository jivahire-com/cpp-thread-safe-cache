
set(TARGET arm_intrinsic)

add_library(${TARGET} INTERFACE)

if("$ENV{REPO_APP_TOOLCHAIN}" STREQUAL "i386-pc-windows-msvc")
    target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/win32/inc)
elseif("$ENV{REPO_APP_TOOLCHAIN}" STREQUAL "arm-eabi-aarch")
    target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cortex_m7/inc)
endif()

add_library(ms::lib::arm_intrinsic ALIAS ${TARGET})
