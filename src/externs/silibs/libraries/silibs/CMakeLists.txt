set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/silibs)
set(TARGET_DIR_SILIBS_COMMON ${REPO_ROOT_DIR}/.externs/repo/silibs/libraries/common)
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
    gpio
    gtimer
    i2c
    icc_mhu
    idhw
    intu
    ioss
    lib_i3c
    mhuv3
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

foreach(SILIB ${SILIB_LIST})
  message("Adding silibs/${SILIB} ...")
  add_subdirectory(${TARGET_DIR}/libraries/${SILIB} ${LIB_OUTS_DIR}/${SILIB})
endforeach()

add_subdirectory(${TARGET_DIR_SILIBS_COMMON}/idsw ${LIB_OUTS_DIR}/idsw)
add_subdirectory(${TARGET_DIR_SILIBS_COMMON}/idsw_kng ${LIB_OUTS_DIR}/idsw_kng)

# Add the libraries where we either implement our own source or our own cmakes
add_subdirectory(kng_silibs_platform ${LIB_OUTS_DIR}/kng_silibs_platform)
add_subdirectory(spi_bridge ${LIB_OUTS_DIR}/spi_bridge)
add_subdirectory(rng ${LIB_OUTS_DIR}/rng)

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
