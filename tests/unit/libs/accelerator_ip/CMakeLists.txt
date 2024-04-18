set(TARGET accelerator_ip.test)

set(SOURCES
    accelerator_ip_test.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE 
        atu
        idsw
        smmu    
        ms::lib::accelerator_ip
        ms::lib::utils
        ms::cmocka::testrunner
        kng_silibs_platform
        sdm_init
        tower_vab
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    idsw_get_die_id
    configure_vab_system_addr_map
    smmu_enable_access
    atu_map
    atu_unmap
    smmu_enable_access_check
    configure_sdmss_system_addr_map
    mmio_write32
    mmio_update32
    debug_print
    critical_print
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
)