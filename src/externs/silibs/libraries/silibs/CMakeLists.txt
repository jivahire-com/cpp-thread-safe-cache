set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/silibs)
set(TARGET_DIR_SILIBS_COMMON ${REPO_ROOT_DIR}/.externs/repo/silibs_common)
set(LIB_OUTS_DIR ${CMAKE_BINARY_DIR}/src/externs/silibs/libraries)

message("----------------------------------")
message("----- Adding Silibs CMAKEs -------")

#Silibs arm gcc version is older than MSCP arm. Hence for forcing enum size
#that's needed for MSCP unit test case ,passing this flag
add_compile_options(-DMSCP_SILIBS_COMPILATION)

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

add_subdirectory(${TARGET_DIR_SILIBS_COMMON}/idsw ${LIB_OUTS_DIR}/idsw)

# Add the libraries where we either implement our own source or our own cmakes
add_subdirectory(kng_silibs_platform)
add_subdirectory(spi_bridge)


# Optimize for DDRSS silibs image size
# DDRSS_PHY_TRAIN_FW_BUILTIN_ENABLE
#   0: Store the PHY training FW in the external RAM/FLASH
#   1: Built-in the PHY training FW as part of the SCP image
target_compile_options (ddrss PRIVATE "-Os" "-DDDRSS_COMPILE_DEBUG_LEVEL=DDRSS_DEBUG_LEVEL_CRITICAL" "-DDDRSS_PHY_TRAIN_FW_BUILTIN_ENABLE=0")
target_compile_options (pciess PRIVATE "-Os")
target_compile_options (pcie PRIVATE "-Os")

# Add math library for DDRSS silibs. It uses float calculation for timing
if (arm-eabi-aarch)
  target_link_libraries (ddrss PRIVATE m)
elseif(i386-pc-windows-msvc)
  # Windows automatically links math functions from the C runtime library, so no need to link with m.lib
  message("Windows build detected - skipping explicit linking of math library.")
endif()
message("----------------------------------")
