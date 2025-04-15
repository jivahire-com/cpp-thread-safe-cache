set(TARGET kng.service.telemetry.cmpnt.in_band_tlm)

add_library(${TARGET}
    STATIC
        mts_manager.c
        ddr_manager.c
        element_schema.c
        in_band_tlm_cmpnt.c
        package_creation.c
        snsr_fifo_debug.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}

)

target_link_libraries(${TARGET}
    PUBLIC

    PRIVATE
        tlm.service.cmpnt.interfaces
        fwlib.lib.assert
        fwlib.lib.status
        fwlib.lib.utils
        fwlib.lib.interface.platform.threadx
        ms::lib::error_codes
        ms::lib::event_trace::trace
        ms::lib::interface::in_band_telemetry_ddr
        ms::service::mts


)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
