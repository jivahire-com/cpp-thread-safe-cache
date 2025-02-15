#
# crash dump unit test
#
set(TARGET crash_dump.test)

set(SOURCES
    test_crash_dump.cpp
    test_crash_dump_cli.cpp
    test_crash_dump_mocks.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(${TARGET}
    PRIVATE
        ${CRASH_DUMP_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.assert
    fwlib.lib.crashDump
    fwlib.service.cli
    ms::build_data
    ms::cmocka::testrunner
    ms::cmsis::interface
    ms::lib::cortex_m7::nvic
    ms::lib::crash_dump
    ms::lib::icc_platform_defines
    ms::lib::threadx
    silibs::idhw
    silibs::kng_silibs_platform
    silibs::gpio
    silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    __NVIC_SetPriority
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
    atu_svc_accel_atu_addr
    fpfw_icc_base_recv
    fpfw_icc_base_send_sync
    _txe_mutex_create
    _tx_mutex_get
    _tx_mutex_put
    gpio_set_output
    crash_dump_context
    initialize_semaphore
    memcpy
    nvic_get_current_irq
    wait_for_semaphore
    release_semaphore
    idhw_is_single_die_boot_en
    idsw_get_platform_sdv
)