# define target for power service
set(TARGET kng.service.telemetry)

add_library(${TARGET}
    STATIC
		telmain_rim.c
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
        silibs::sensor_ram_bridge
        silibs::fuse
    PRIVATE
        fwlib.dfwk
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.threadx
        fwlib.lib.status
        ms::lib::error_codes
        ms::service::sensor_fifo		
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::telemetry ALIAS ${TARGET})
