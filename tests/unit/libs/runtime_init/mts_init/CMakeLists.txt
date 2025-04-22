set(TARGET mts_init.test)

set(SOURCES
    test_mts_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/mts_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        ms::service::mts
        fwlib.service.mts.cli
        ms::lib::interface::in_band_telemetry_ddr
        ms::lib::icc_platform_defines
        ms::cmocka::testrunner
        silibs::icc_mhu
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_init_get_handle
    idsw_get_cpu_type
    idsw_get_die_id
    idhw_is_single_die_boot_en
    mts_init
    mts_cli_svc_initialize
)
