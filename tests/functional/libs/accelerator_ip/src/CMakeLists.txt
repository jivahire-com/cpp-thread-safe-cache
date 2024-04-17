set(TARGET ms_test_accelerator_ip_lib)

# Add target for the service
add_library(
    ${TARGET}
        STATIC
            accelerator_ip_cli.c
)

target_include_directories(
    ${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/../inc
            ${REPO_ROOT_DIR}/.externs/repo/silibs/libraries/common/silibs_platform/include
            ${REPO_ROOT_DIR}/.externs/azpkg/kng_soc_mason_csr_collateral.0.5.1/ap_addr_map/registerheader

        PRIVATE
            
)

target_link_libraries(
    ${TARGET}
        PRIVATE
        silibs::atu
        silibs::idsw
        silibs::kng_silibs_platform
)

add_library(ms::tests::functional::lib::accelerator_ip::lib ALIAS ${TARGET})



