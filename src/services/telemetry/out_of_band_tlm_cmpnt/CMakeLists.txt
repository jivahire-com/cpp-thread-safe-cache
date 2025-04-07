set(TARGET kng.service.telemetry.cmpnt.out_of_band_tlm)

add_library(${TARGET}
    STATIC
        out_of_band_tlm_cmpnt.c
        pldm_comm.c
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
        fwlib.lib.pldm.protocol
        fwlib.service.pldm
        ms::lib::error_codes
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
