set(TARGET scp_pcie_manager.test)

set(SOURCES
    pcie_manager_unit_tests.cpp
    pcie_mocks.c
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
    ms::driver::pcie
    ms::lib::error_handler
    ms::lib::threadx
    ms::mocks::threadx
    ms::service::pcie_manager
    silibs_platform
)

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::service::pcie_manager,INCLUDE_DIRECTORIES>)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    DfwkAsyncRequestInititalize
    DfwkAsyncRequestSetCompletionRoutine
    DfwkInterfaceSendAsync
    _txe_queue_create
    _txe_thread_create
    _txe_queue_send
    _txe_queue_receive
    pcie_dfwk_init
)
