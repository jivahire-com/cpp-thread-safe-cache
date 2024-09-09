set(TARGET ap_core_init.test)

set(SOURCES
    test_ap_core_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/ap_core_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        fwlib.service.iccBase
        ms::client_interface::startup_shutdown
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::service::ap_core
        ms::service::atu_service
        silibs::atu
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
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
    ap_core_init
    ap_core_interface_init
    atu_translate_address
    fpfw_init_get_handle
    idsw_get_platform_sdv
    sos_register_ssi
    idsw_get_die_id
)
