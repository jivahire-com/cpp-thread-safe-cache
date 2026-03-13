# Copyright (c) Microsoft Corporation. All rights reserved.

#
# crash dump unit test
#
set(TARGET crash_dump.test)

set(SOURCES
    test_crash_dump.cpp
    test_crash_dump_cli.cpp
    test_crash_dump_mocks.cpp
    test_crash_dump_transfer.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(${TARGET}
    PRIVATE
        ${CRASH_DUMP_SOURCE_DIR}/inc
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
        PLDM_DRV_WORKAROUND
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.assert
    fwlib.lib.crashDump
    fwlib.lib.pldm.protocol
    fwlib.service.cli
    fwlib.service.pldm
    ms::build_data
    ms::client_interface::startup_shutdown
    ms::cmocka::testrunner
    ms::cmsis::interface
    ms::driver::pldm    # PLDM_DRV_WORKAROUND
    ms::lib::cortex_m7::nvic
    ms::lib::crash_dump
    ms::lib::icc_platform_defines
    ms::lib::telemetry::fuses
    ms::lib::threadx
    ms::mocks::silibs_platform
    silibs::idhw
    silibs::ddrss
    silibs::kng_silibs_platform
    silibs::gpio
    silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    __NVIC_SetPriority
    DfwkAsyncRequestComplete
    DfwkAsyncRequestInitialize
    DfwkAsyncRequestSetCompletionRoutine
    DfwkClientInterfaceOpen
    DfwkDeviceInitialize
    DfwkInterfaceInitialize
    DfwkInterfaceSendAsync
    DfwkQueueInitialize
    FPFwCDInitMemoryPool
    FPFwCDMemPoolOverrideCacheFlush
    FPFwCDMemPoolOverrideCacheInvalidate
    FPFwCDInitDumpDescriptor
    FPFwCDDumpDescriptorSetMutexCtx
    FPFwCDDumpDescriptorOverrideMutexLock
    FPFwCDDumpDescriptorOverrideMutexUnlock
    FPFwCDInitDumpFile
    FPFwCDDumpFileOverrideInValidMemory
    FPFwCDDumpFileOverrideInValidCsrMemory
    FPFwCDDumpFileOverrideInValidGlobalMemory
    FPFwCDInitDumpManager
    FPFwCDOverridePrintf
    FPFwCDDumpManagerSetPreDumpCallback
    FPFwCDDumpManagerSetPostDumpCallback
    FPFwCDDumpManagerOverrideGetCurTime
    FpFwCliRegisterTable
    FpFwCliPrint
    FPFwCDCrashDumpHandler
    CdRegisterMMIORegisterSet
    CdRegisterAddress32
    CdRegisterAddress64
    CdRegisterAddress32PointerArray
    CdRegisterCallback
    CdRegisterRegisterSet
    CdRegisterCustomChunk
    atu_svc_accel_atu_addr
    fpfw_icc_base_recv
    fpfw_icc_base_recv_sync
    fpfw_icc_base_send
    fpfw_icc_base_send_sync
    fpfw_init_get_handle
    idsw_get_cpu_type
    idsw_get_die_id
    _txe_mutex_create
    _txe_timer_create
    _txe_timer_deactivate
    _tx_mutex_get
    _tx_mutex_put
    _tx_thread_sleep
    gpio_set_output
    gpio_get_input
    get_crash_dump_region_address
    crash_dump_context
    initialize_semaphore
    memcpy
    mmio_read32
    nvic_get_current_irq
    wait_for_semaphore
    release_semaphore
    idhw_is_single_die_boot_en
    idsw_get_platform_sdv
    crash_dump_get_is_dump_complete
    crash_dump_get_dump_size
    pldm_drv_raise_platform_event   # PLDM_DRV_WORKAROUND
    # fpfw_pldm_service_raise_platform_event    # !PLDM_DRV_WORKAROUND
    system_info_is_warm_start
    tlm_fuses_get_ecid
    get_timestamp_from_cd
    update_timestamp_in_cd
    utc_sync_client_get_current_time_epoch_ms
    gtimer_get_timestamp_ms
)