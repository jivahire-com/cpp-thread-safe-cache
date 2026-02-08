# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET vab_driver.test)

set(SOURCES
    vab_mocks.c
    vab_unit_tests.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.utils
    fwlib.lib.cfg_mgr
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::atomics
    ms::lib::cortex_m7::nvic
    ms::lib::error_handler
    ms::lib::interrupts
    ms::lib::scp_interrupts
    ms::driver::pcie
    ms::driver::vab
    ms::service::health_monitor
    silibs::idsw
    silibs::idsw_kng
    silibs::atu
    silibs::kng_silibs_platform
    silibs::pcie
    silibs::pciess
    silibs_platform
    silibs::vab_init
    silibs::smmu
    silibs::intu
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    atu_find_map
    atu_map
    atu_unmap
    config_get_vab_knobs
    config_get_smmu_vab_knobs
    crash_dump_bug_check
    d2d_error_isr
    hm_submit_cper
    idhw_is_single_die_boot_en
    idsw_get_platform_sdv
    idsw_get_die_id
    nvic_irq_set_isr_with_param
    nvic_irq_clear_pending
    nvic_irq_enable
    pcie_ss_set_base
    pciess_get_entity
    pciess_probe
    pciess_clear_handler
    ras_agent_probe
    ras_arm_agent_set_base
    ras_arm_record_to_cper
    ras_print_record
    rpss_irq_callback
    tower_fmu_handler
    vab_get_1p_parity_status
    vab_fabric_error_trigger_by_type
    vab_init
    vabss_intu_probe
    vabss_intu_clear
    vab_ras_trigger_by_type
)
