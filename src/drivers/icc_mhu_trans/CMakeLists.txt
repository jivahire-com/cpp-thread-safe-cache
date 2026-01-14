# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.driver.icc_mhu_trans)

add_library(${TARGET}
    STATIC
    src/mhu_icc_transport.c
    src/mhu_icc_transport_events.c
)


target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.assert
        fwlib.lib.utils
        ms::lib::arm_intrinsic
        ms::lib::crash_dump
        ms::lib::debug
        ms::lib::event_trace::trace
        silibs_platform
    PUBLIC
        fwlib.lib.interface.platform.interrupts
        fwlib.lib.interface.platform.timers
        fwlib.lib.interface.platform.threadx
        fwlib.driver.interface.iccTransport
        fwlib.service.icc_dispatcher
        ms::cmsis::interface
        silibs::icc_mhu
)

if (arm-eabi-aarch)
    add_compile_definitions(PLATFORM_CACHING_ENABLED)
endif()

add_library(ms::driver::icc_mhu_trans ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
