# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET tower_init.test)

set(SOURCES
    test_tower_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/tower_init.c
)

add_executable(${TARGET} ${SOURCES})
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")


target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.utils
    fwlib.lib.assert
    fwlib.lib.init
    fwlib.lib.interface.platform.dbgprint
    fwlib.service.iccBase
    ms::cmocka::testrunner
    ms::lib::crash_dump
    ms::lib::tower
    ms::lib::hsp_firmware_headers
    ms::lib::system_info
    ms::service::health_monitor
    silibs::idsw_kng
    silibs::idhw
    silibs::kng_silibs_platform
    silibs::tower_sequence
    silibs::silibs_utils
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    fpfw_init_get_handle
    idsw_get_die_id
    tower_init
    hm_register_error_domain
)
