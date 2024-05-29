# define target for power service
set(TARGET kng.service.power_service)

add_library(${TARGET}
    STATIC
        power_fuses.c
        power_hw_int.c
        power_hw_int_gpio.c
        power_hw_int_telemetry.c
        power_knobs.c
        power_main.c
        power_runconfig.c
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
        silibs::kng_silibs_platform
        silibs::pvt
        silibs::sensor_ram_bridge
    PRIVATE
        fwlib.dfwk
        fwlib.platform.errorhandler
        fwlib.platform.threadx
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::power ALIAS ${TARGET})
