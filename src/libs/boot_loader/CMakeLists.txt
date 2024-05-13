set(TARGET boot_loader)

# Add target for the service
add_library(
    ${TARGET} STATIC
        kingsgate_boot.c
        unpack_image.c
)

target_include_directories(
    ${TARGET} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${REPO_ROOT_DIR}/.externs/repo/zlib
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../src/externs/zlib/zlib		
)

target_link_libraries(
    ${TARGET}
    PRIVATE
    ms::lib::zlib
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::boot_loader ALIAS ${TARGET})
