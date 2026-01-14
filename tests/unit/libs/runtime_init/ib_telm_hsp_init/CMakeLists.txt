# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET ib_telm_hsp_init.test)

set(SOURCES
    test_ib_telm_hsp_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/ib_telm_hsp_init.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.service.iccBase
        ms::cmocka::testrunner
        ms::lib::hsp_firmware_headers
        ms::lib::interface::in_band_telemetry_ddr
        silibs::idsw
        silibs::idsw_kng
)

# Generic threadx lock port provided by fwlibs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_LOCK "fwlib.lib.platform.impl.lock.threadx")

# Generic ports implemented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    fpfw_icc_base_send_sync
    fpfw_init_get_handle
    idsw_get_die_id
)
