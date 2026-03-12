# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET hw_version_init_scp.test)

set(SOURCES
    test_hw_version_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/hw_version_scp_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        ms::cmocka::testrunner
        silibs_platform
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        ms::lib::utils
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    idhw_set_sid_base
    idsw_set_cpu_type
    idsw_set_die_id
    idhw_get_soc_id
    idhw_get_die_id
    idhw_get_platform_id_from_hw
    idsw_set_platform_sdv
)
