
set(TARGET in_band_telemetry_ddr)

add_library(${TARGET} INTERFACE)

target_link_libraries(${TARGET}
    INTERFACE
        ms::lib::utils
        ms::service::atu_service
)

target_include_directories(
    ${TARGET}
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::lib::interface::in_band_telemetry_ddr ALIAS ${TARGET})
