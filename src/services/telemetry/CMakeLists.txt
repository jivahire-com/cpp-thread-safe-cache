add_library(tlm.service.cmpnt.interfaces INTERFACE)

target_include_directories(tlm.service.cmpnt.interfaces
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/data_proc_tlm_cmpnt/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/exec_tlm_cmpnt/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/in_band_tlm_cmpnt/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/out_of_band_tlm_cmpnt/inc
)

# add dependencies that are common to all internal telemetry libs
target_link_libraries(tlm.service.cmpnt.interfaces
    INTERFACE
        ms::service::sensor_fifo
        ms::lib::threadx
        kng_silibs_platform
        fwlib.lib.status
        fwlib.lib.assert
        fwlib.lib.utils
        ms::lib::interface::in_band_telemetry_ddr
        kng.service.telemetry.cmpnt.event_trace
    )

add_subdirectory(event_trace_cmpnt)
add_subdirectory(data_proc_tlm_cmpnt)
add_subdirectory(exec_tlm_cmpnt)
add_subdirectory(in_band_tlm_cmpnt)
add_subdirectory(out_of_band_tlm_cmpnt)


set(TARGET kng.service.telemetry)

add_library(${TARGET}
    STATIC
        telemetry_service.c
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
        fwlib.lib.assert
        fwlib.lib.status
        fwlib.lib.utils
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.threadx
        ms::lib::error_codes
        ms::lib::interface::in_band_telemetry_ddr
        kng.service.telemetry.cmpnt.exec
        kng.service.telemetry.cmpnt.data_process
        kng.service.telemetry.cmpnt.in_band_tlm
        kng.service.telemetry.cmpnt.out_of_band_tlm
        ms::service::sensor_fifo
        kng_silibs_platform

)

add_library(ms::service::telemetry ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
