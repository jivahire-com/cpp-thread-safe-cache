# SCP Crash dump
set(TARGET kng.lib.crash_dump.scp)

add_library(${TARGET}
    STATIC
        src/crash_dump.c
        src/crash_dump_init.c
        src/crash_dump_overrides.c
        src/crash_dump_payload_core.c
        src/crash_dump_payload_msft.c
        src/crash_dump_payload_threadx.c
        src/scp/crash_dump_init_scp.c
        src/scp/crash_dump_gpio.c
        src/scp/crash_dump_overrides.c
        src/scp/crash_dump_payload.c
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.crashDump
        ms::build_data
        ms::cmsis::interface
        silibs::kng_silibs_platform
        ms::lib::threadx
        ms::lib::error_codes
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::crash_dump::scp ALIAS ${TARGET})


# MCP Crash dump
set(TARGET kng.lib.crash_dump.mcp)

add_library(${TARGET}
    STATIC
        src/crash_dump.c
        src/crash_dump_init.c
        src/crash_dump_overrides.c
        src/crash_dump_payload_core.c
        src/crash_dump_payload_msft.c
        src/crash_dump_payload_threadx.c
        src/mcp/crash_dump_gpio.c
        src/mcp/crash_dump_overrides.c
        src/mcp/crash_dump_payload.c
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.crashDump
        ms::build_data
        ms::cmsis::interface
        silibs::kng_silibs_platform
        ms::lib::threadx
        silibs::gpio
        silibs_platform
        ms::lib::error_codes
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::crash_dump::mcp ALIAS ${TARGET})
