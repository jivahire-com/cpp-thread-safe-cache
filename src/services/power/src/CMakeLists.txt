# define target for power service
set(TARGET kng.service.power_service)

add_library(${TARGET}
    STATIC
        pid_resource.c
        power_cap.c
        power_cli_requests.c
        power_distribution.c
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
        power_remote_die.c
        power_runconfig.c
        power_stub.c
        power_timer.c
        power_warmstart.c
        power_vcpu_calc.c
)

# save this for enabling debug optimization
# target_compile_options(${TARGET} PUBLIC "-Og")

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
        fwlib.service.iccBase
        ms::client_interface::startup_shutdown
        ms::lib::cortex_m7::atomics
        ms::lib::event_trace::trace
        ms::lib::crash_dump
        ms::lib::error_codes
        ms::lib::icc_platform_defines
        ms::lib::warm_start
        ms::service::sensor_fifo
        register_headers
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::power ALIAS ${TARGET})
