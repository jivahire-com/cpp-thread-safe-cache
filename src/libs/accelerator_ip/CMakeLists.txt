set(TARGET ms_accelerator_ip_lib)

# Add target for the service
add_library(
    ${TARGET}
        STATIC
            accelerator_ip_boot_status.c
            accelerator_ip_configs.c
            accelerator_ip_init.c
            accelerator_ip_isolation.c
            accelerator_ip_knobs.c
            accelerator_ip_mcp.c
            accelerator_ip_warm_reset.c
)

target_include_directories(
    ${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/inc
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/inc_priv
)

target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:fwlib.service.iccBase,INCLUDE_DIRECTORIES>)

target_link_libraries(
    ${TARGET}
    PUBLIC
        fwlib.lib.status
        ms::client_interface::startup_shutdown
        ms::lib::accel_intr
        ms::lib::icc_platform_defines
        silibs::accelip_ss_init
        silibs::atu
        silibs::kng_silibs_platform
        silibs::sdm_command_desc
        silibs::sdm_init
    PRIVATE
        fwlib.dfwk
        fwlib.driver.mboxIccTransport
        fwlib.lib.cfg_mgr
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.utils
        fwlib.service.icc_dispatcher
        fwlib.service.iccBase
        ms::client_interface::startup_shutdown
        ms::driver::icc_mhu_trans
        ms::lib::boot_status
        ms::lib::config_manager
        ms::lib::crash_dump
        ms::lib::debug
        ms::lib::error_codes
        ms::lib::hsp_firmware_headers
        ms::lib::register_headers
        ms::lib::utils
        ms::lib::system_info
        ms::service::atu_service
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::mhuv3
        silibs::pcr_vab
        silibs::pcr_rpss
        silibs::smmu
        silibs::tower_sdmss
        silibs::tower_system_control
        silibs::tower_utility
        silibs::tower_vab
        silibs::vab_pcr_init
        silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::accelerator_ip ALIAS ${TARGET})
