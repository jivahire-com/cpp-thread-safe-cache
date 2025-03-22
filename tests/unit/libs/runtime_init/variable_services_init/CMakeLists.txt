set(TARGET variable_services_init.test)

set(SOURCES
    test_variable_services_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/variable_services_init.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.init
    fwlib.lib.interface.platform.dbgprint
    fwlib.lib.utils
    idhw
    silibs::idsw
    silibs::idsw_kng
    kng_silibs_platform
    ms::cmocka::testrunner
    ms::lib::variable_services
    ms::lib::hsp_firmware_headers
    fwlib.service.iccBase
)

# Generic threadx lock port provided by fwlibs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_LOCK "fwlib.lib.platform.impl.lock.threadx")

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "ms::lib::threadx")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    fpfw_init_get_handle
    idhw_get_die_id
)
