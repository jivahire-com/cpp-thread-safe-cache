set(TARGET variable_services.test)

set(SOURCES
    test_variable_services.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.utils
        fwlib.service.iccBase
        kng_silibs_platform
        ms::cmocka::testrunner
        ms::lib::hsp_firmware_headers
        ms::lib::variable_services
        ms::lib::system_info
)

# @todo remove after completing task https://azurecsi.visualstudio.com/Dev/_workitems/edit/1933312/
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:fwlib.service.iccBase,INCLUDE_DIRECTORIES>)

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "ms::lib::threadx")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    fpfw_icc_base_send_recv_sync
    system_info_is_hsp_present
)
