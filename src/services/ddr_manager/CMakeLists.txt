set(TARGET ddr_manager_service)
set(LIBS PUBLIC fwlib.dfwk fwlib.lib.assert)

# Add target for the service
add_library(${TARGET}
    STATIC
        ddr_manager_acpi_smbios_tables.c
        ddr_manager_bwl.c
        ddr_manager_gpio.c
        ddr_manager_i3c.c
        ddr_manager_memory_map.c
        ddr_manager_platform.c
        ddr_manager_polling.c
        ddr_manager_telemetry.c
        ddr_manager_top.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PRIVATE
        silibs::ddr_i3c
        silibs::ddrss
        silibs::idhw
        silibs::kng_silibs_platform
        silibs::platform
        silibs::ras_arm
        fwlib.dfwk
        fwlib.lib.init
        ms::lib::crash_dump::scp
        ms::lib::error_codes
        ms::lib::i3c_controller
        ms::service::sds_service
    PUBLIC
        fwlib.platform.dfwkhost.threadx
        fwlib.dfwk
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.threadx
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::ddr_manager ALIAS ${TARGET})
