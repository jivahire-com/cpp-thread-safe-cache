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
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        ms::client_interface::startup_shutdown
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::service::scp_startup_shutdown
        silibs::atu
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::idhw
        silibs::idsw
)

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    DfwkAsyncRequestInitialize
    fpfw_init_get_handle
    sos_init
    sos_interface_init
    sos_register_ssi
    sos_start_phase
)
