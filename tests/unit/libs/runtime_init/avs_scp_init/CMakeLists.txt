# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET avs_scp_init.test)

set(SOURCES
    test_avs_scp_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/avs_scp_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.linkedlist
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::scp_interrupts
        ms::lib::threadx
        scp_avs
        silibs::avs
        silibs_platform
        silibs::idsw_kng
        silibs::kng_silibs_platform
        ms::lib::utils
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    idsw_get_die_id
    scp_avs_init
    scp_avs_interface_initialize
    fpfw_init_get_handle
)
