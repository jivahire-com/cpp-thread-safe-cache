set(TARGET kng.lib.sensor_thresholds)

add_library(${TARGET} INTERFACE)

target_include_directories(${TARGET}
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

add_library(ms::lib::sensor_thresholds ALIAS ${TARGET})

