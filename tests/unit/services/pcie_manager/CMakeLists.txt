set(TARGET scp_pcie_manager.test)

set(SOURCES
    pcie_manager_unit_tests.cpp
    pcie_mocks.c
    pcie_silibs_mocks.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.utils
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
    silibs::kng_silibs_platform
    silibs::pcie
    silibs::pciess
    silibs::pcr_rpss    
    silibs_platform
)

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::service::pcie_manager,INCLUDE_DIRECTORIES>)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")


lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    DfwkAsyncRequestInitialize
    DfwkAsyncRequestSetCompletionRoutine
    DfwkInterfaceSendAsync
    _txe_queue_create
    _txe_thread_create
    _txe_queue_send
    _txe_queue_receive
    _txe_event_flags_create
    _txe_event_flags_set
    _txe_event_flags_get
    _txe_event_flags_delete
    idsw_get_die_id
    pcie_dfwk_init
    pciess_rp_get_link_train_done
    scp_pcie_get_manager_context
    send_sync_get_rpss_entity
)
