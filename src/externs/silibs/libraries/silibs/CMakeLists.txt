set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/silibs)
set(LIB_OUTS_DIR ${CMAKE_BINARY_DIR}/src/externs/silibs/libraries)

message("----------------------------------")
message("----- Adding Silibs CMAKEs -------")

add_compile_options(-I${TARGET_DIR}/libraries/common/silibs_platform/include
                    -I${TARGET_DIR}/registerheader_overrides
)

# Disable warnings for shared libs
add_compile_options(-w)

# Use the `silibs_manifest` file next to this CMAKE to define which libraries to bring in
file (STRINGS "silibs_manifest" SILIB_LIST)

foreach(SILIB ${SILIB_LIST})
  message("Adding silibs/${SILIB} ...")
  add_subdirectory(${TARGET_DIR}/libraries/${SILIB} ${LIB_OUTS_DIR}/${SILIB})
endforeach()

# Add the libraries where we either implement our own source or our own cmakes
add_subdirectory(kng_silibs_platform)
add_subdirectory(spi_bridge)

message("----------------------------------")
