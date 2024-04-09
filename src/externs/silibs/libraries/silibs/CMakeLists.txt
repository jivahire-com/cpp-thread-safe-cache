set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/silibs)
set(LIB_OUTS_DIR ${CMAKE_BINARY_DIR})

message("----------------------------------")
message("----- Adding Silibs CMAKEs -------")

add_compile_options(-I${TARGET_DIR}/libraries/common/silibs_platform/include
                    -I${TARGET_DIR}/registerheader_overrides
                    -I${REPO_ROOT_DIR}/.externs/azpkg/kng_soc_mason_csr_collateral.0.5.1/scp_addr_map/registerheader
                    -I${REPO_ROOT_DIR}/.externs/azpkg/kng_soc_mason_csr_collateral.0.5.1/mcp_addr_map/registerheader
                    -I${REPO_ROOT_DIR}/.externs/azpkg/kng_soc_mason_csr_collateral.0.5.1/ap_addr_map/registerheader
                    -I${REPO_ROOT_DIR}/.externs/azpkg/kng_soc_mason_csr_collateral.0.5.1/hsss_dfp_addr_map/registerheader
)

# Disable warnings for shared libs
add_compile_options(-w)

# Use the `silibs_manifest` file next to this CMAKE to define which libraries to bring in
file (STRINGS "silibs_manifest" SILIB_LIST)

foreach(SILIB ${SILIB_LIST})
  # TODO: Currently RAS and CMN800 libs don't build on clang
  # ADO: 1749700
  if(SILIB MATCHES "ras_|cmn800|cml" AND "$ENV{REPO_APP_TOOLCHAIN}" STREQUAL "i386-pc-windows-msvc")
    message("Skipping silibs/${SILIB} ...")
    continue()
  endif()

  message("Adding silibs/${SILIB} ...")
  add_subdirectory(${TARGET_DIR}/libraries/${SILIB} ${LIB_OUTS_DIR}/${SILIB})
endforeach()

message("----------------------------------")
