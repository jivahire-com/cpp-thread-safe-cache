# define target for power service
set(TARGET kng.service.power_service)

add_library(${TARGET}
    STATIC
        pid_resource.c
        power_cap.c
        power_cli_requests.c
        power_events.c
        power_fuses.c
        power_hw_int.c
        power_hw_int_gpio.c
        power_hw_int_telemetry.c
        power_hw_int_vrs.c
        power_knobs.c
        power_loops.c
        power_loops_control.c
        power_loops_telemetry.c
        power_main.c
        power_runconfig.c
        power_stub.c
        power_timer.c
        power_warmstart.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PUBLIC
        ms::lib::corebits
        silibs::dvfs
        silibs::gpio
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::pvt
        silibs::sensor_ram_bridge
        silibs::fuse
    PRIVATE
        fwlib.dfwk
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.threadx
        fwlib.lib.status
        ms::client_interface::startup_shutdown
        ms::lib::event_trace::trace
        ms::lib::crash_dump::scp
        ms::lib::error_codes
        ms::lib::warm_start
        ms::service::sensor_fifo
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::power ALIAS ${TARGET})
