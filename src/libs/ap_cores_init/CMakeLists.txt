set(TARGET ms_ap_cores_init_lib)

# Add target for the service
add_library(
    ${TARGET}
        STATIC
            ap_cores_init.c
)

target_include_directories(
    ${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        fwlib.lib.assert
        ms::lib::register_headers
        silibs::atu
        silibs::platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::ap_cores_init ALIAS ${TARGET})
