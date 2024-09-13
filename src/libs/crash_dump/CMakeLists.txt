# SCP Crash dump
set(TARGET kng.lib.crash_dump)

set (SOURCES 
        src/crash_dump.c
        src/crash_dump_cli.c
        src/crash_dump_gpio.c
        src/crash_dump_init.c
        src/crash_dump_overrides.c
        src/crash_dump_payload_core.c
        src/crash_dump_payload_msft.c
        src/crash_dump_payload_threadx.c
)

if("$ENV{REPO_APP_TOOLCHAIN}" STREQUAL "i386-pc-windows-msvc")
    set (SOURCES ${SOURCES} src/win32/crash_dump_dummy_symbols.c)
endif()

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        fwlib.lib.crashDump
        ms::lib::debug
    PRIVATE
        fwlib.lib.assert
        fwlib.service.cli
        ms::build_data
        ms::cmsis::interface
        ms::lib::threadx
        ms::lib::error_codes
        silibs::gpio
        silibs::kng_silibs_platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::crash_dump ALIAS ${TARGET})
