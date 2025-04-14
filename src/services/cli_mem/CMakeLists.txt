set(TARGET kng.service.cli.mem)

# Add the cli_mem source files to the build
set(SOURCES
    cli_mem.c
)

# Create a library for the cli_mem CLI
add_library(${TARGET} STATIC ${SOURCES})

# Link dependencies
target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.interface.platform.dbgprint
        fwlib.service.cli
        silibs::atu
        silibs::kng_silibs_platform
        silibs_platform
)

# Include directories
target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add an alias for the library
add_library(ms::service::mem::cli ALIAS ${TARGET})

# Enable linting, formatting, and coverage
lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})