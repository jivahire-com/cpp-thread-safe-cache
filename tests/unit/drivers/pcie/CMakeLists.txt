set(TARGET scp_pcie_driver.test)

set(SOURCES
    pcie_driver_mocks.c
    pcie_driver_unit_tests.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.utils
    fwlib.dfwk
    fwlib.platform.dfwkhost.threadx
    kng_silibs_platform
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::mocks::threadx
    ms::driver::pcie
    silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::driver::pcie,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    DfwkQueueEnqueueRequest
    DfwkAsyncRequestComplete
    _txe_timer_create
    _txe_timer_delete
)
