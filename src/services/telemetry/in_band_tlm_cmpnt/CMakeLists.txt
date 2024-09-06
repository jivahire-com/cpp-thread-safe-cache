set(TARGET kng.service.telemetry.cmpnt.in_band_tlm)

add_library(${TARGET}
    STATIC
        in_band_tlm_cmpnt.c
        schema_handling.c
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
        ms::lib::error_codes
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
