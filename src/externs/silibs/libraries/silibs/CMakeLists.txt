set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/silibs)
set(LIB_OUTS_DIR ${CMAKE_BINARY_DIR})

message("----------------------------------")
message("----- Adding Silibs CMAKEs -------")


add_compile_options(-I${TARGET_DIR}/libraries/common/silibs_platform/include
                    -I${TARGET_DIR}/registerheader_overrides
                    # REPO_APP_PATH_kng_soc_mason_csr_collateral env variable set by start.ps1 when pulling the kng_soc_mason_csr_collateral package
                    -I$ENV{REPO_APP_PATH_kng_soc_mason_csr_collateral}/scp_addr_map/registerheader
                    -I$ENV{REPO_APP_PATH_kng_soc_mason_csr_collateral}/mcp_addr_map/registerheader
                    -I$ENV{REPO_APP_PATH_kng_soc_mason_csr_collateral}/ap_addr_map/registerheader
                    -I$ENV{REPO_APP_PATH_kng_soc_mason_csr_collateral}/hsss_dfp_addr_map/registerheader
)

# Disable warnings for shared libs
add_compile_options(-w)

# Use the `silibs_manifest` file next to this CMAKE to define which libraries to bring in
file (STRINGS "silibs_manifest" SILIB_LIST)

foreach(SILIB ${SILIB_LIST})
  message("Adding silibs/${SILIB} ...")
  add_subdirectory(${TARGET_DIR}/libraries/${SILIB} ${LIB_OUTS_DIR}/${SILIB})
endforeach()

message("----------------------------------")
