set(TARGET sel_init.scp.test)

set(SOURCES
    test_sel_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/sel_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        fwlib.service.cli
        fwlib.service.iccBase
        fwlib.service.pldm
        ms::cmocka::testrunner
        ms::lib::error_codes
        ms::lib::system_info
        ms::lib::threadx
        ms::service::sel_mgr
        silibs::icc_mhu
        silibs::idhw
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_init_get_handle
    idsw_get_die_id
    idsw_get_cpu_type
    fpfw_icc_base_recv
    DfwkDeviceInitialize
    DfwkQueueInitialize
    DfwkInterfaceInitialize
    DfwkInterfaceOpen
    FpFwCliRegisterTable
    FpFwLockInitialize
    FpFwLockAcquire
    FpFwLockRelease
    idhw_is_single_die_boot_en
)

set(TARGET sel_init.mcp.test)

set(SOURCES
    test_sel_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/sel_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_compile_definitions(
    ${TARGET}
    PRIVATE
    MCP_RUNTIME_INIT
    PLDM_DRV_WORKAROUND
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        fwlib.service.cli
        fwlib.service.iccBase
        fwlib.service.pldm
        ms::cmocka::testrunner
        ms::driver::pldm    # PLDM_DRV_WORKAROUND
        ms::lib::error_codes
        ms::lib::system_info
        ms::lib::threadx
        ms::service::sel_mgr
        silibs::icc_mhu
        silibs::idhw
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_init_get_handle
    idsw_get_die_id
    idsw_get_cpu_type
    fpfw_icc_base_recv
    pldm_drv_register_platform_event_ready_notification  # PLDM_DRV_WORKAROUND
    # fpfw_pldm_service_register_platform_event_ready_notification # !PLDM_DRV_WORKAROUND
    DfwkDeviceInitialize
    DfwkQueueInitialize
    DfwkInterfaceInitialize
    DfwkInterfaceOpen
    DfwkAsyncRequestSetCompletionRoutine # PLDM_DRV_WORKAROUND
    FpFwCliRegisterTable
    FpFwLockInitialize
    FpFwLockAcquire
    FpFwLockRelease
    idhw_is_single_die_boot_en
)
