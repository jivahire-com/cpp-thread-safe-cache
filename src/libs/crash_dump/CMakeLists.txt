# SCP Crash dump
set(TARGET kng.lib.crash_dump.scp)

add_library(${TARGET}
    STATIC
        src/crash_dump.c
        src/crash_dump_init.c
        src/crash_dump_overrides.c
        src/crash_dump_register_standard.c
        src/scp/crash_dump_init_scp.c
        src/scp/crash_dump_gpio_scp.c
        src/scp/crash_dump_overrides_scp.c
        src/scp/crash_dump_register_scp.c
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.assert
    fwlib.lib.crashDump
    ms::lib::threadx
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
        src/crash_dump_register_standard.c
        src/mcp/crash_dump_gpio_mcp.c
        src/mcp/crash_dump_overrides_mcp.c
        src/mcp/crash_dump_register_mcp.c
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.assert
    fwlib.lib.crashDump
    ms::lib::threadx
    silibs::gpio
    silibs_platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::crash_dump::mcp ALIAS ${TARGET})
