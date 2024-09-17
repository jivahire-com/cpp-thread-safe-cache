set(TARGET scmi_init.test)

set(SOURCES
    test_scmi_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/scmi_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        ms::cmocka::testrunner
        ms::driver::scmi_drv
        ms::lib::threadx
        ms::service::scp_startup_shutdown
        silibs::atu
        silibs::idsw_kng
        silibs::kng_silibs_platform
)

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "ms::lib::threadx")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_init_get_handle
    scmi_set_apcore_interface
)
