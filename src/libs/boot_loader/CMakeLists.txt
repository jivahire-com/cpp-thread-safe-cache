set(TARGET boot.loader.interface)

add_library(${TARGET} INTERFACE)

target_include_directories(${TARGET} INTERFACE
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/inc
                           ${REPO_ROOT_DIR}/.externs/repo/zlib
                           ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../src/externs/zlib/zlib
)

add_library(ms::boot_loader::interface ALIAS ${TARGET})

enabled_on(arm-eabi-aarch)

set(TARGET boot_loader)

# Add target for the service
add_library(
    ${TARGET} STATIC
        kingsgate_boot.c
        unpack_image.c
        dummy_sys_calls.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
    register_headers
    fwlib.lib.mboxPrimitives
    ms::boot_loader::interface
    ms::cmsis::interface
    ms::lib::hsp_firmware_headers
    ms::lib::system_info
    ms::lib::zlib
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::boot_loader ALIAS ${TARGET})
