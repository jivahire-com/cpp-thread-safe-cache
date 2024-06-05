#
# crash dump unit test for SCP
#
set(TARGET crash_dump.scp.test)

set(SOURCES
    test_crash_dump_scp.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.crashDump
    ms::cmocka::testrunner
    ms::lib::crash_dump::scp
    ms::lib::threadx
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
    _txe_mutex_create
    _tx_mutex_get
    _tx_mutex_put
)

#
# crash dump unit test for MCP
#
set(TARGET crash_dump.mcp.test)

set(SOURCES
    test_crash_dump_mcp.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.crashDump
    ms::cmocka::testrunner
    ms::lib::crash_dump::mcp
    ms::lib::threadx
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
    _txe_mutex_create
    _tx_mutex_get
    _tx_mutex_put
    gpio_set_output
)
