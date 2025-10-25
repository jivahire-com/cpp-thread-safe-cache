set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/silibs)
set(TARGET_DIR_SILIBS_COMMON ${REPO_ROOT_DIR}/.externs/repo/silibs/libraries/common)
set(TARGET_DIR_SILIBS_SDM_LIBS ${REPO_ROOT_DIR}/.externs/repo/silibs/libraries/sdm/libraries/)
set(LIB_OUTS_DIR ${CMAKE_BINARY_DIR}/src/externs/silibs/libraries)

message("----------------------------------")
message("----- Adding Silibs CMAKEs -------")

#Silibs arm gcc version is older than MSCP arm. Hence for forcing enum size
#that's needed for MSCP unit test case ,passing this flag
add_compile_options(-DMSCP_SILIBS_COMPILATION)

add_compile_options(-I${TARGET_DIR}/libraries/common/silibs_platform/include
                    -I${TARGET_DIR}/registerheader_overrides
                    -I$ENV{REPO_APP_PATH_cded_register_header}
)
if (arm-eabi-aarch)
  option(PLATFORM_CACHING_ENABLED "Platform provides cache flush/invalidate functionality." ON)
elseif(i386-pc-windows-msvc)
  option(PLATFORM_CACHING_ENABLED "Platform provides cache flush/invalidate functionality." OFF)
endif()


# Disable warnings for shared libs
add_compile_options(-w)

set(SILIB_LIST
    accelip_ss_init
    atu
    avs
    clocks_sequence
    cml
    cmn800
    cmn800_ras_sequence
    cmn800_sequence
    cmn800_skus
    cmsdk_wd
    d2d_counter_sync
    ddr_i3c
    ddrss
    dmac
    dvfs
    fuse
    fuse_common
    gpio
    gtimer
    i2c
    icc_mhu
    idhw
    ift
    intu
    ioss
    ioss_init
    lib_i3c
    mhuv3
    mesh_d2d_telemetry
    pcie
    pciess
    pcr_clock_config
    pcr_css
    pcr_d2dss
    pcr_ddrss
    pcr_ioss
    pcr_msxp
    pcr_periph
    pcr_rpss
    pcr_vab
    pik_clock
    ppu_v1
    pvt
    ras_arm
    ras_framework
    ras_mcp
    ras_mscp
    ras_scp
    sdm_ext_interrupts
    sdm_init
    semaphore
    sensor_ram_bridge
    spi_bridge
    smmu
    tower_cdedss
    tower_d2dss
    tower_ddrss
    tower_fabric
    tower_ioss
    tower_mgmtap
    tower_perip
    tower_rpss
    tower_sdmss
    tower_sequence
    tower_system_control
    tower_utility
    tower_vab
    usb_init
    vab_init
    vab_pcr_init
)

set(SILIB_SDM_LIST
  cded_fw_libs_platform
  cded_interrupts
)

foreach(SILIB ${SILIB_LIST})
  message("Adding silibs/${SILIB} ...")
  add_subdirectory(${TARGET_DIR}/libraries/${SILIB} ${LIB_OUTS_DIR}/${SILIB})
endforeach()

foreach(SILIB_SDM ${SILIB_SDM_LIST})
  message("Adding silibs/${SILIB_SDM} ...")
  add_subdirectory(${TARGET_DIR_SILIBS_SDM_LIBS}/${SILIB_SDM} ${LIB_OUTS_DIR}/${SILIB_SDM})
endforeach()

add_subdirectory(${TARGET_DIR_SILIBS_COMMON}/idsw ${LIB_OUTS_DIR}/idsw)
add_subdirectory(${TARGET_DIR_SILIBS_COMMON}/idsw_kng ${LIB_OUTS_DIR}/idsw_kng)
add_subdirectory(${TARGET_DIR_SILIBS_COMMON}/silibs_utils ${LIB_OUTS_DIR}/silibs_utils)

# Add the libraries where we either implement our own source or our own cmakes
add_subdirectory(kng_silibs_platform ${LIB_OUTS_DIR}/kng_silibs_platform)
add_subdirectory(rng ${LIB_OUTS_DIR}/rng)

# Optimize for DDRSS silibs image size
# DDRSS_PHY_TRAIN_FW_BUILTIN_ENABLE
#   0: Store the PHY training FW in the external RAM/FLASH
#   1: Built-in the PHY training FW as part of the SCP image
# DDRSS_PHY_PIE_CODE_DATA_BUILTIN_ENABLE
#   0: Store the PHY PIE code and data in the external RAM/FLASH
#   1: Built-in the PHY PIE code and data as part of the SCP image
target_compile_options (
  ddrss PRIVATE
    "-Os"
    "-DDDRSS_PHY_TRAIN_FW_BUILTIN_ENABLE=0"
    "-DDDRSS_PHY_PIE_CODE_DATA_BUILTIN_ENABLE=0"
    "-DDDRSS_KEY_LOADING_OVERRIDE=1"
)
target_compile_options (cmn800_skus PRIVATE "-DCMN800_FULL_MESH_TABLE=0")
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
