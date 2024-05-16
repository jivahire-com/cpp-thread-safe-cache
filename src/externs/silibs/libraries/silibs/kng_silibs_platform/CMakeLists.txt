message("----------------------------------")
message("-- Adding Silibs KNG Platform ----")

set(TARGET extern.silibs.kng_silibs_platform)

add_library(${TARGET} INTERFACE)

target_include_directories(
    ${TARGET}
        SYSTEM INTERFACE
            ${TARGET_DIR}/libraries/kng_silibs_platform/include
)

target_link_libraries(${TARGET}
    INTERFACE
        silibs::platform
        ms::lib::register_headers
)

add_library(kng_silibs_platform ALIAS ${TARGET})
add_library(silibs::kng_silibs_platform ALIAS ${TARGET})
