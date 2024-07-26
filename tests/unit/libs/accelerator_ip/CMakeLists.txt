set(TARGET accelerator_ip.test)

set(SOURCES
    test_accelerator_ip.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        atu
        silibs::idsw
        silibs::idsw_kng
        smmu
        pcr_rpss
        sdm_init
        tower_vab
        ms::lib::accelerator_ip
        ms::lib::utils
        ms::cmocka::testrunner
        kng_silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    atu_map
    atu_unmap
    configure_cdedss_hsp_system_addr_map
    configure_cdedss_vab_system_addr_map
    FpFwAssert
    debug_print
    critical_print
    idsw_get_die_id
    idsw_get_platform_sdv
    pcr_rpss_configure_clock
    pcr_rpss_deassert_por_reset
    pcr_rpss_init_entity
    accelip_ss_init
)
