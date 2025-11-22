set(TARGET kng.lib.ddrss_init)

add_library(${TARGET}
    STATIC
        src/ddrss.c
        src/ddr_atu_map.c
        src/prod_ddrss_isr.c
        src/ddr_err_inj.c
)

# Auto-generate ddrss_reserved_regions_version.h before building this target.
set(GEN_VERSION_SCRIPT ${CMAKE_SOURCE_DIR}/tools/pwsh/generate_ddrss_reserved_regions_version.ps1)
set(SILIBS_ROOT ${REPO_ROOT_DIR}/.externs/repo/silibs)
set(RESERVED_REGIONS_HEADER ${SILIBS_ROOT}/soc_fw_shared/include/memory_map/ddrss_reserved_regions.h)
set(GENERATED_DDRSS_VERSION_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/inc/ddrss_reserved_regions_version.h)

add_custom_command(
    OUTPUT ${GENERATED_DDRSS_VERSION_HEADER}
    COMMAND pwsh -NoLogo -ExecutionPolicy Bypass -File ${GEN_VERSION_SCRIPT} -Force -VerboseOutput
    DEPENDS ${GEN_VERSION_SCRIPT} ${RESERVED_REGIONS_HEADER}
    COMMENT "Generating ddrss_reserved_regions_version.h from silibs tag"
    VERBATIM
)

add_custom_target(ddrss_reserved_regions_version
    DEPENDS ${GENERATED_DDRSS_VERSION_HEADER}
)

add_dependencies(${TARGET} ddrss_reserved_regions_version)

if (i386-pc-windows-msvc)
    set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
    message("Windows build detected - spoofing interrupt library.")
endif()

target_link_libraries(${TARGET}
    PUBLIC
    ms::lib::crash_dump
    ms::lib::ddr_manager
    ms::lib::register_headers
    ms::lib::variable_services
    ras_arm
    silibs::cmn800
    silibs::ddr_i3c
    silibs::ddrss
    silibs::intu
    PRIVATE
    ms::lib::cortex_m7::nvic
    ms::lib::arm_intrinsic
    ms::lib::boot_status
    ms::lib::crash_dump
    ms::lib::ddr_manager
    ms::lib::gtimer
    ms::lib::scp_interrupts
    ms::lib::system_info
    ms::service::health_monitor
    ms::service::sel_mgr
    silibs::platform
    kng_silibs_platform
    atu
    silibs::cmsdk_wd
    silibs::icc_mhu
    silibs::idhw
    silibs::idsw
    silibs::idsw_kng
    silibs::pcr_ddrss
    silibs::ras_arm
    fwlib.dfwk
    fwlib.lib.cfg_mgr
    fwlib.lib.utils
    fwlib.lib.assert
    fwlib.lib.interface.platform.interrupts
)

# Enable DDRSS new CPER implementation
add_compile_definitions(DDRSS_USE_NEW_CPER)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        $<TARGET_PROPERTY:ms::service::health_monitor,INCLUDE_DIRECTORIES>
        ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::ddrss_init ALIAS ${TARGET})
