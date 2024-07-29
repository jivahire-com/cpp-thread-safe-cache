set(TARGET kng.lib.hsp_firmware_headers)

add_library(${TARGET} INTERFACE)
target_include_directories(${TARGET}
    INTERFACE
        $ENV{REPO_APP_PATH_kingsgate.hsp.headers.release}/release
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
add_library(ms::lib::hsp_firmware_headers ALIAS ${TARGET})
