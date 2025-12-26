set(TARGET ddrss.test)

set(SOURCES
    ddrss_mocks.c
    test_ddr_sdl.cpp
    test_ddrss.cpp
    test_ddr_err_inj.cpp
    test_ddr_atu_map.cpp
    
)

# Compile test
add_executable(${TARGET} ${SOURCES})

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.lib.ddrss_init,INCLUDE_DIRECTORIES>)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::cortex_m7::nvic
        ms::lib::ddrss_init
        ms::lib::register_headers
        ms::lib::scp_interrupts
        ms::lib::ddrss_init
        silibs::atu
        silibs::ddrss
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::ras_arm
        silibs::ras_framework
        fwlib.lib.interface.platform.interrupts
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    FpFwAssert
    atu_map
    atu_unmap
    config_get_fips_kat_en
    cmn800_generate_ddr_mc_map_from_cached_config
    crash_dump_bug_check
    ddrss_atu_map_cfg_space
    ddrss_atu_unmap_cfg_space
    ddrss_atu_map_fips_ns_space
    ddrss_atu_map_fips_rt_space
    ddrss_atu_unmap_fips_space
    ddrss_clear_phy_interrupt_status
    ddrss_convert_ras_rec_to_cper
    ddrss_ddr_intu_clear_destination_interrupt
    ddrss_ddr_intu_clear_interrupt
    ddrss_ddr_intu_get_interrupt_dest_enable
    ddrss_get_config
    ddrss_get_phy_interrupt_status
    ddrss_get_phy_interrupt_status
    ddrss_get_phy_training_failure
    ddrss_get_ras_agent
    ddrss_init
    ddrss_get_telemetry_record
    ddrss_inject_media_ca_err
    ddrss_inject_media_data_err
    ddrss_mc_event_clear_interrupt
    ddrss_mc_intu_get_interrupt_dest_enable
    ddrss_mc_intu_get_interrupt_status
    ddrss_physical_to_media_addr
    ddrss_probe_ras_agent
    hm_submit_cper
    idhw_is_single_die_boot_en
    idhw_is_stepping_a1
    intu_get_interrupt_status
    mmio_read32
    mmio_write32
    nvic_irq_clear_pending
    nvic_irq_enable
    nvic_irq_set_isr_with_param
    post_led_status
    ras_arm_agent_probe
    ras_print_record
    system_info_is_warm_start
    ddr_err_inj_ecc_ce
    ddr_err_inj_ecc_ue
    ddr_err_inj_mainline_traffic_ce
    ddr_err_inj_mainline_traffic_ue
    ddr_err_inj_media_patrol_scrub_ce
    ddr_err_inj_media_patrol_scrub_ue
    ddr_err_inj_fecq_fedb_data_array_ue
    ddr_err_inj_fedb_merge_data_ue
    ddr_err_inj_fedb_merge_data_ce
    ddr_err_inj_fedb_merge_data_parity_ue
    ddr_err_inj_fedb_merge_strobe_ue
    ddr_err_inj_fedb_merge_strobe_parity_ue
    ddr_err_inj_fedb_strobe_array_ue
    ddr_err_inj_ca_parity_persistent
    ddr_err_inj_ca_parity_transient
    ddr_err_rh_counters_sram_parity
    ddr_err_rh_drfm_sram_parity
    ddr_err_xts_aes_keystore_ce
    ddr_err_xts_aes_keystore_ue
    ddr_err_bcp_read_addr_not_in_ddr
    ddr_err_bcp_read_blocked_by_pas
    ddr_err_bcp_write_addr_not_in_ddr
    ddr_err_bcp_write_blocked_by_pas
    ddr_err_bcp_chi_unsupported_opcode
    ddr_err_inj_mrdp_parity_ue
    ddr_i3c_interface_read_spd_nvm_data
    ddr_i3c_interface_write_spd_nvm_data
    log_sel_event
    get_dimm_vendor_id
    sdl_get_mem_ctx
    variable_service_initialize_ctx
    variable_service_sync_get_variable
    variable_service_unlock_get_var_ctx
)
