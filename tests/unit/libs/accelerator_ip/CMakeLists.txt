set(TARGET accelerator_ip.test)

set(SOURCES
    test_accelerator_ip.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE 
        atu
        idsw
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
    FpFwAssert
    debug_print
    critical_print
    mmio_write32
    mmio_update32 
    idsw_get_die_id
    idsw_get_platform_sdv
    smmu_enable_access
    smmu_enable_access_check
    configure_sdmss_system_addr_map
    sdm_init_smmu_connection_seq
    sdm_init_set_ecam_base
    sdm_init_set_rciep_bdf
    sdm_init_enable_ecam
    sdm_init_set_emcpu_initvtor
    sdm_init_enable_fence
    sdm_init_deassert_nsysreset
    sdm_init_trigger_memory_init_blocking
    sdm_init_enable_itcm_ecc
    sdm_init_enable_dtcm_ecc
    sdm_init_disable_cpu_wait
    sdm_init_set_rcec_bdf
    configure_vab_system_addr_map
    deassert_pcr_reset
    vab_pcr_init
    pcr_rpss_configure_clocks
    pcr_rpss_deassert_por_reset
    pcr_rpss_verify
)