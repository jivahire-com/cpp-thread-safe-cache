# Enable this library only for the ARM target because this library needs Silibs
# support which is only enabled for ARM target.
set(TARGET ms_accelerator_ip_lib)

# Add target for the service
add_library(
    ${TARGET}
        STATIC
            accelerator_ip_configs.c
            accelerator_ip_mcp.c
            accelerator_ip.c
)

target_include_directories(
    ${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/inc
            ${REPO_ROOT_DIR}/src/libs/utils/include

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc_priv
)

# @todo remove after completing task https://azurecsi.visualstudio.com/Dev/_workitems/edit/1933312/
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:fwlib.service.iccBase,INCLUDE_DIRECTORIES>)

target_link_libraries(
    ${TARGET}
    PUBLIC
        ms::lib::accel_intr
        silibs::accelip_ss_init
        silibs::atu
        silibs::kng_silibs_platform
        silibs::sdm_command_desc
        silibs::sdm_init
    PRIVATE
        fwlib.lib.assert
        fwlib.driver.mboxIccTransport
        fwlib.lib.cfg_mgr
        fwlib.lib.init
        fwlib.service.icc_dispatcher
        fwlib.service.iccBase
        ms::lib::crash_dump
        ms::lib::debug
        ms::lib::error_codes
        ms::lib::hsp_firmware_headers
        ms::lib::register_headers
        ms::lib::utils
        ms::lib::system_info
        silibs::idsw
        silibs::idsw_kng
        silibs::pcr_vab
        silibs::pcr_rpss
        silibs::smmu
        silibs::tower_cdedss # TODO: ADO 1831262: Remove me after completing the ADO
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
