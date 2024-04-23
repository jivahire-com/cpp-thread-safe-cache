# Enable this library only for the ARM target because this library needs Silibs
# support which is only enabled for ARM target.
set(TARGET ms_accelerator_ip_lib)

# Add target for the service
add_library(
    ${TARGET}
        STATIC
            accelerator_ip_configs.c
            accelerator_ip.c
)

target_include_directories(
    ${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/inc
            ${REPO_ROOT_DIR}/.externs/repo/silibs/libraries/common/silibs_platform/include
            ${REPO_ROOT_DIR}/.externs/azpkg/kng_soc_mason_csr_collateral.0.5.1/ap_addr_map/registerheader
            ${REPO_ROOT_DIR}/.externs/repo/silibs/libraries/tower_utility/include

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc_priv
)

target_link_libraries(
    ${TARGET}
    PUBLIC
        silibs::atu
        silibs::kng_silibs_platform
        silibs::sdm_init
    PRIVATE
        fwlib.assert
        ms::lib::debug
        silibs::idsw
        silibs::pcr_vab
        silibs::pcr_rpss
        silibs::smmu
        silibs::smmu_voyager
        silibs::tower_sdmss
        silibs::tower_system_control
        silibs::tower_vab
        silibs::vab_pcr_init
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::accelerator_ip ALIAS ${TARGET})
