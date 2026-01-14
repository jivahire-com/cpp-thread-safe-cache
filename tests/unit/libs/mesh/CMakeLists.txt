# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET mesh.test)

set(SOURCES
    test_mesh.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PUBLIC
        fwlib.lib.interface.platform.dbgprint

    PRIVATE
        cml
        cmn800
        cmn800_sequence
        cmn800_ras_sequence
        fwlib.lib.cfg_mgr
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.lib.interface.platform.interrupts
        idhw
        silibs::idsw
        silibs::idsw_kng
        kng_silibs_platform
        ms::cmocka::testrunner
        ms::lib::mesh
        ms::lib::cortex_m7::nvic
        ms::lib::d2d_cntr_sync
        ms::lib::scp_interrupts
        ms::lib::variable_services
        silibs_platform
        silibs::spi_bridge
        ms::lib::hsp_firmware_headers
        fwlib.service.iccBase
        ras_arm
        atu
)

# @todo remove after completing task https://azurecsi.visualstudio.com/Dev/_workitems/edit/1933312/
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:fwlib.service.iccBase,INCLUDE_DIRECTORIES>)

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "ms::lib::threadx")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    cmn800_sequence
    idhw_is_single_die_boot_en
    idhw_get_die_id
    idsw_get_die_id
    nvic_irq_set_isr_with_param
    nvic_irq_clear_pending
    nvic_irq_enable
    d2dss_sequence
    cmn800_sequence_svp_updates
    fpfw_icc_base_send_recv_sync
    fpfw_icc_base_send_sync
    system_info_is_hsp_present
    interrupt_handler_mesh_ras_error
    ras_arm_agent_setup_entity
    ras_arm_agent_init
    ras_arm_agent_enable_signaling_by_type
    ras_arm_agent_enable_fhi
    ras_arm_agent_enable_logging
    ras_arm_agent_set_base
    ras_arm_agent_probe
    ras_print_record
    ras_arm_agent_trigger_by_type
    atu_translate_address
    atu_map
    atu_unmap
    config_get_mesh_d2d_override
    cmn800_get_mesh_sam_cfg_knob
    cmn800_get_mesh_ras_cfg_knob
    get_ccg_knob_defaults
    get_default_d2d_cfg
    process_mesh_binary_from_spi
    system_info_is_warm_start
    config_get_d2d_ecc_ce_counter
    mmio_read32
    mmio_write32
    variable_service_initialize_ctx
    variable_service_sync_set_variable
    variable_service_sync_get_variable
    memset
    memcmp
    mscp_exp_spi_synchronize_dies
    crash_dump_bug_check
    d2d_cntr_sync_enable
    cmn800_generate_ddr_mc_map_from_snf_config
    cmn800_error_injection
    cmn800_pseudo_fault_error_injection
    hm_submit_cper
    ras_arm_agent_notify_warm
    ras_arm_agent_set_record_counter_thresholds_by_index
    get_hns_sds_vector_from_hns_sparring
    idhw_is_stepping_a1
    config_get_mesh_hnf_cbusy_limit_ctl
    config_get_mesh_hnf_cbusy_resp_ctl
    config_get_mesh_hnf_cbusy_sn_ctl
    config_get_mesh_hnf_cbusy_write_limit_ctl
    config_get_mesh_hnf_cbusy_mode_ctl
    config_get_mesh_hnf_aux_ctl
    config_get_mesh_hnf_aux_ctl_1
    config_get_mesh_hnf_cfg_ctl
    config_get_mesh_hnf_lbt_cfg_ctl
    config_get_mesh_hnf_lbt_aux_ctl
    config_get_mesh_hnf_lbt_cbusy_ctl
    config_get_mesh_hnf_pocq_alloc_class_dedicated
    config_get_mesh_hnf_pocq_alloc_class_max_allowed
    config_get_mesh_hnf_pocq_alloc_class_contended_min
    config_get_mesh_hnf_pocq_alloc_misc_max_allowed
    config_get_mesh_hnf_class_ctl
    config_get_mesh_hnf_pocq_qos_class_ctl
    config_get_mesh_hnf_class_pocq_arb_weight_ctl
    config_get_mesh_hnf_class_retry_weight_ctl
    config_get_mesh_hnf_pocq_misc_retry_weight_ctl
    config_get_mesh_sbsx_cbusy_limit_ctl
    config_get_mesh_hni_cfg_ctl_knob
    config_get_mesh_hni_aux_ctl_knob
    config_get_mesh_hnt_dn_domain_cxra
    config_get_mesh_rni_cfg_ctl_knob
    config_get_mesh_rni_aux_ctl_knob
    config_get_mesh_rnd_cfg_ctl_knob
    config_get_mesh_rnd_aux_ctl_knob
    config_get_mesh_rni_qos_cfg_knob
    config_get_mesh_rnd_qos_cfg_knob
    config_get_mesh_mxp_qos_cfg_knob
    config_get_mesh_RAS_Error_Detection_Disable
    config_get_mesh_RAS_Error_Deferment_Disable
    config_get_mesh_RAS_Uncorrected_Error_Int_Disable
    config_get_mesh_RAS_Fault_Handling_Int_Disable
    config_get_mesh_RAS_Corrected_Error_Int_Disable
    config_get_mesh_RAS_Parity_Error_Disable
    config_get_por_ccg_ha_aux_ctl
    config_get_por_ccg_ha_cfg_ctl
    config_get_por_ccg_ha_cxprtcl_link0_ctl
    config_get_por_ccg_ra_cfg_ctl
    config_get_por_ccg_ra_aux_ctl
    config_get_por_ccg_ra_ccprtcl_link0_ctl
    config_get_por_ccg_ra_cbusy_limit_ctl
    config_get_por_ccla_aux_ctl
    config_get_d2d_pll_divider
    config_get_d2d_ref_divider
    config_get_d2d_pll_fb_divider
    config_get_d2d_ecc_cfg
    config_get_d2d_tx_interface_clk_alignment
    config_get_d2d_sleep_cfg
    config_get_d2d_sleep_cfg_entry
    config_get_d2d_rxcal_find_goodlanes_skip
    config_get_d2d_close_fb_wa
    config_get_d2dss_system_counter_delay
    config_get_cmn_sam_config
    config_get_cxl_params_die0
    config_get_cxl_params_die1
    config_get_cmn_uma_arsm_htg_wa_disabled
    config_get_a1_mesh_hnf_cbusy_resp_ctl
    config_get_a1_mesh_hnf_cbusy_sn_ctl
    config_get_a1_mesh_hnf_cbusy_write_limit_ctl
    config_get_a1_mesh_hnf_cbusy_mode_ctl
    config_get_a1_mesh_hnf_aux_ctl
    config_get_a1_mesh_hnf_aux_ctl_1
    config_get_a1_mesh_hnf_cfg_ctl
    config_get_a1_mesh_hnf_lbt_cfg_ctl
    config_get_a1_mesh_hnf_lbt_aux_ctl
    config_get_a1_mesh_hnf_lbt_cbusy_ctl
    config_get_a1_mesh_hnf_pocq_alloc_class_dedicated
    config_get_a1_mesh_hnf_pocq_alloc_class_max_allowed
    config_get_a1_mesh_hnf_pocq_alloc_class_contended_min
    config_get_a1_mesh_hnf_pocq_alloc_misc_max_allowed
    config_get_a1_mesh_hnf_pocq_qos_class_ctl
    config_get_a1_por_ccg_ha_aux_ctl
    config_get_a1_por_ccg_ha_cfg_ctl
    config_get_a1_por_ccg_ra_cfg_ctl
    config_get_a1_por_ccg_ra_aux_ctl
    config_get_a1_por_ccg_ra_ccprtcl_link0_ctl
    config_get_a1_por_ccg_ra_cbusy_limit_ctl
)
