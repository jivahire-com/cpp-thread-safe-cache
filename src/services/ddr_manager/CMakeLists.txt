set(TARGET ddr_manager_service)
set(LIBS PUBLIC fwlib.dfwk fwlib.assert)

# Add target for the service
add_library(${TARGET}
    STATIC
        ddr_manager_acpi_smbios_tables.c
        ddr_manager_memory_map.c
        ddr_manager_polling.c
        ddr_manager_top.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PRIVATE
        silibs::ddrss
        silibs::platform
        idhw
        fwlib.dfwk
        fwlib.init
    PUBLIC
        fwlib.platform.dfwkhost.threadx
        fwlib.dfwk
        fwlib.platform.errorhandler
        fwlib.platform.threadx
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::ddr_manager ALIAS ${TARGET})
