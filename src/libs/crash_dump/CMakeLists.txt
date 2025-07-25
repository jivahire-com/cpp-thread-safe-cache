# SCP Crash dump
set(TARGET kng.lib.crash_dump)

set (SOURCES 
        src/crash_dump.c
        src/crash_dump_accel.c
        src/crash_dump_cli.c
        src/crash_dump_context.c
        src/crash_dump_dfwk.c
        src/crash_dump_gpio.c
        src/crash_dump_icc.c
        src/crash_dump_init.c
        src/crash_dump_overrides.c
        src/crash_dump_payload_core.c
        src/crash_dump_payload_msft.c
        src/crash_dump_payload_threadx.c
        src/crash_dump_pldm.c
        src/crash_dump_status.c
        src/crash_dump_stream.c
        src/crash_dump_events.c
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
        fwlib.dfwk
        fwlib.lib.crashDump
        fwlib.service.iccBase
        ms::lib::debug
        ms::lib::error_codes
        ms::lib::event_trace::trace
        ms::lib::threadx
        ms::service::atu_service
        silibs::atu
        silibs::sdm_command_desc
        silibs::semaphore
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.cfg_mgr
        fwlib.lib.init
        fwlib.lib.pldm.protocol
        fwlib.service.cli
        fwlib.service.pldm
        kng.lib.hsp_firmware_headers
        ms::build_data
        ms::client_interface::startup_shutdown
        ms::cmsis::interface
        ms::lib::cortex_m7::nvic
        ms::lib::icc_platform_defines
        ms::lib::register_headers
        ms::lib::system_info
        silibs::gpio
        silibs::icc_mhu
        silibs::idhw
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
