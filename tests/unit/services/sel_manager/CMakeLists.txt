# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET sel_manager.test)

set(SOURCES
    sel_manager_tests.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(
    ${TARGET}
        PRIVATE
            ${REPO_ROOT_DIR}/src/services/sel_manager
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       PLDM_DRV_WORKAROUND
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.service.pldm
        ms::cmocka::testrunner
        ms::service::sel_mgr
        silibs::idsw
        silibs::idsw_kng
        silibs::icc_mhu
        fwlib.service.iccBase
        ms::lib::utils
        ms::lib::error_codes
        ms::driver::pldm    # PLDM_DRV_WORKAROUND
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    FpFwLockInitialize
    FpFwLockAcquire
    FpFwLockRelease
    idsw_get_die_id
    idsw_get_cpu_type
    fpfw_icc_base_send
    fpfw_icc_base_recv
    pldm_drv_register_platform_event_ready_notification # PLDM_DRV_WORKAROUND
    pldm_drv_raise_platform_event                       # PLDM_DRV_WORKAROUND
    # fpfw_pldm_service_register_platform_event_ready_notification  # !PLDM_DRV_WORKAROUND
    # fpfw_pldm_service_raise_platform_event                        # !PLDM_DRV_WORKAROUND
    DfwkDeviceInitialize
    DfwkQueueInitialize
    DfwkInterfaceInitialize
    DfwkInterfaceOpen
    DfwkAsyncRequestInitialize
    DfwkAsyncRequestSetCompletionRoutine
    DfwkInterfaceSendAsync
    DfwkAsyncRequestComplete
)
