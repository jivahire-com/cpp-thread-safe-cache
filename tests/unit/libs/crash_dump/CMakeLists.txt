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
    ms::lib::crash_dump
    ms::lib::icc_platform_defines
    ms::lib::threadx
    silibs::kng_silibs_platform
    silibs::gpio
    silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
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
    FPFwCDRegisterAddress32
    FPFwCDCrashDumpHandler
    CdRegisterMMIORegisterSet
    CdRegisterAddress32
    CdRegisterAddress64
    CdRegisterAddress32PointerArray
    CdRegisterCallback
    CdRegisterRegisterSet
    fpfw_icc_base_recv
    fpfw_icc_base_send_sync
    _txe_mutex_create
    _tx_mutex_get
    _tx_mutex_put
    gpio_set_output
    GetCrashDumpConfig
)