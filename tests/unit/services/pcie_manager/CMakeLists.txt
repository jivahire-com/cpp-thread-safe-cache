set(TARGET scp_pcie_manager.test)

set(SOURCES
    pcie_manager_unit_tests.cpp
    pcie_mocks.c
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
    ms::lib::accelerator_ip
    ms::lib::error_handler
    ms::lib::threadx
    ms::lib::variable_services
    ms::mocks::threadx
    ms::service::pcie_manager
    ms::service::atu_service
    silibs::kng_silibs_platform
    silibs::pcie
    silibs::pciess
    silibs::pcr_rpss
    silibs_platform
)

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

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
    # DFWK mocks
    DfwkAsyncRequestInitialize
    DfwkAsyncRequestSetCompletionRoutine
    DfwkClientInterfaceOpen
    DfwkInterfaceSendAsync
    DfwkInterfaceSendSync

    # Thread X mocks
    _txe_event_flags_create
    _txe_event_flags_delete
    _txe_event_flags_get
    _txe_event_flags_set
    _txe_queue_create
    _txe_queue_receive
    _txe_queue_send
    _txe_thread_create
    _tx_thread_sleep
    _txe_timer_create
    _txe_timer_delete

    # Silibs mocks
    idsw_get_die_id
    ras_print_record

    # Driver mocks
    pcie_dfwk_init
    pcie_dfwk_interface_init
    scp_pcie_get_manager_context

    # SCP support mocks
    accel_is_isolation_enabled
    config_get_overlake_rpss_index_secondary_soc
    config_get_overlake_rpss_index_primary_soc
    config_get_enable_overlake_sbr_workaround
    crash_dump_bug_check
    memcpy
    memset
    pcie_phyfw_wait_load_event
    system_info_get_board_id
    variable_serv_copy_to_rmss_ram
    variable_service_async_set_variable
    variable_service_initialize_ctx
)
