set(TARGET scp_pcie_driver.test)

set(SOURCES
    pcie_dfwk_mocks.c
    pcie_misc_mocks.c
    pcie_silibs_mocks.c
    pcie_driver_unit_tests.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.utils
    fwlib.dfwk
    fwlib.platform.dfwkhost.threadx
    fwlib.lib.cfg_mgr
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::lib::interrupts
    ms::lib::scp_interrupts
    ms::lib::system_info
    ms::mocks::threadx
    ms::driver::pcie
    ms::driver::vab
    silibs::atu
    silibs::idsw
    silibs::idsw_kng
    silibs::intu
    silibs::kng_silibs_platform
    silibs::pcie
    silibs::pciess
    silibs::pcr_rpss
    silibs::tower_rpss
    silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::driver::pcie,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    # Mock external MSCP interfaces
    enable_vab_isrs
    system_info_get_board_id
    # Config manager mocks
    config_get_cxl_params_die0
    config_get_cxl_params_die1
    config_get_pcie_configuration_mirroring
    # DFWK mocks
    DfwkQueueEnqueueRequest
    DfwkAsyncRequestComplete
    # Threadx mocks
    _txe_timer_create
    _txe_timer_delete
    _txe_timer_deactivate
    _txe_semaphore_create
    _txe_semaphore_get
    _txe_semaphore_put
    # Silibs mocks begin
    atu_map
    idsw_get_die_id
    pciess_rp_get_link_train_done
    intu_get_interrupt_status
    idsw_get_platform_sdv
    oi_pcie_rp_dbi_hide_dpc_cap
    oi_pcie_ss_set_laattr_rp_overrides
    oi_pcie_rp_dbi_set_write_cacheability
    oi_pcie_rp_dbi_set_read_cacheability
    pciess_get_entity
    pciess_config_entity
    pciess_config_ss_for_bifur
    pciess_deassert_por_reset
    pciess_rp_clear_intus
    pciess_rp_initiate_link_training
    pciess_phys_sram_init_done
    pciess_rps_clear_intus
    pciess_rps_pre_rp_ready_init
    pciess_rp_ready
    pciess_rps_ready
    pciess_rps_post_rp_ready_init
    pciess_probe
    pciess_phys_program_fw
    pcie_rp_vsecras_clear_rasdp_error_mode
    ras_pcie_vsecras_agent_probe
    ras_pcie_dtim_agent_probe
    ras_pcie_ltim_agent_probe
)
