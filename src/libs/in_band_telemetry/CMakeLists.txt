
set(TARGET in_band_telemetry_ddr)

add_library(${TARGET} INTERFACE)

target_link_libraries(${TARGET}
    INTERFACE
        ms::lib::utils
)

target_include_directories(
    ${TARGET}
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}
            # neccessary for easy access to shared memory maps
            ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include
)

add_library(ms::lib::interface::in_band_telemetry_ddr ALIAS ${TARGET})
