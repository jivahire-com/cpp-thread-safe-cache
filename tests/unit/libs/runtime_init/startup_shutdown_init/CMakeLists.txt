set(TARGET sos_init.test)

set(SOURCES
    test_startup_shutdown_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/startup_shutdown_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.init
        fwlib.utils
        fwlib.platform.dfwkhost.threadx
        ms::client_interface::startup_shutdown
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::service::scp_startup_shutdown
        silibs::atu
        silibs::idsw
        silibs::kng_silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    DfwkAsyncRequestInititalize    
    fpfw_init_get_handle
    idsw_get_die_id
    sos_init
    sos_interface_init
    sos_register_ssi
    sos_start_phase
)
