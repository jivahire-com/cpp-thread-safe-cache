set(TARGET boot_loader)

set(SOURCES
    kingsgate_boot.c
    unpack_image.c
)

if (arm-eabi-aarch)
set(SOURCES ${SOURCES}
    dummy_sys_calls.c
)
endif()

add_library(${TARGET} ${SOURCES})

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
