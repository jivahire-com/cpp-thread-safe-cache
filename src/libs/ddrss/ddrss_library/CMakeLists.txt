set(TARGET kng.lib.ddrss_init)

add_library(${TARGET}
    STATIC
        src/ddrss.c
        src/ddr_atu_map.c
        src/prod_ddrss_isr.c
        src/ddr_err_inj.c
)

if (i386-pc-windows-msvc)
    set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
    message("Windows build detected - spoofing interrupt library.")
endif()

target_link_libraries(${TARGET}
    PUBLIC
    ms::lib::crash_dump 
    ms::lib::register_headers
    silibs::cmn800
    silibs::ddr_i3c
    silibs::ddrss
    silibs::intu
    PRIVATE
    ms::lib::cortex_m7::nvic
    ms::lib::arm_intrinsic
    ms::lib::scp_interrupts
    silibs::platform
    kng_silibs_platform
    atu
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



target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::ddrss_init ALIAS ${TARGET})
