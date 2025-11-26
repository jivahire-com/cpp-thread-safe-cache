set(TARGET ddr_manager.test)

set(SOURCES
    ddr_manager_bwl_test.cpp
    ddr_manager_i3c_test.cpp
    ddr_manager_polling_test.cpp
    ddr_manager_test.cpp
    ddr_memory_map_test.cpp
    ddr_mocks.cpp
)

set(LIBS
    PRIVATE
        idhw
        ms::cmocka::testrunner
        ms::lib::ddr_manager
        ms::lib::error_handler
        ms::lib::hsp_firmware_headers
        ms::lib::threadx
        ms::mocks::threadx
        ms::lib::config_manager
        ms::lib::stdio_textio
        ms::lib::variable_services
        ms::driver::textio::pl011
        silibs::ddr_i3c
        silibs::atu
        silibs::spi_bridge
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.lib.spinlock
        fwlib.service.iccBase
        ras_arm
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
        ${LIBS}
)

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::lib::ddr_manager,INCLUDE_DIRECTORIES>)

# Defining platform APIs
# set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
# set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.interface.win32")
# set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})
mock_target(${TARGET}
    _tx_thread_sleep
    _txe_mutex_create
    _txe_queue_create
    _txe_thread_create
    _txe_queue_send
    _txe_queue_receive
    _txe_timer_create
    atu_map
    atu_translate_address
    atu_unmap
    config_get_borgens_1gb_ddr_reserve_enable
    config_get_ddrmanager_bwl_polling_en
    copy_single_smbios_type_17
    ddr_create_memory_map
    config_get_erhm_en
    config_get_ras_init_en
    config_get_rh_tlm_service_period_ms
    config_get_ddrmanager_sdl_en
    ddr_create_memory_map
    ddr_i3c_interface_read_spd_nvm_data
    ddr_i3c_interface_read_temp_sensor_mr_reg
    ddr_i3c_interface_read_pmic_power
    ddr_i3c_interface_get_instance
    ddr_manager_platform_is_polling_supported
    ddrss_bandwidth_limiter_config
    ddrss_get_ddrss_mask
    ddrss_get_system_mem_region
    ddrss_get_training_margin_base
    ddrss_is_valid_local_mc
    ddrss_phy_get_training_margin
    dimm_vendor_from_id
    fpfw_icc_base_send_recv_sync
    get_dimm_part_number_string
    get_dimm_serial_number_string
    get_i3c_dimm_detected
    gtimer_prodfw_get_counter
    gtimer_prodfw_get_frequency
    idhw_get_die_id
    idhw_is_single_die_boot_en
    idsw_get_die_id
    idsw_get_platform_sdv
    ift_is_enabled
    mmio_read8
    mmio_read16
    mmio_read32
    mmio_write8
    mmio_write16
    mmio_write32
    mscp_exp_spi_synchronize_dies
    prod_ddrss_get_ras_erg_ce_interrupt_enable
    prod_ddrss_set_ras_erg_ce_interrupt_enable
    prod_ddrss_lib_init
    sds_block_creation
    sds_block_write
    system_info_is_hsp_present
    system_info_is_warm_start
    sensor_fifo_svc_add_dimm_info
    prod_ddrss_get_intr_event_cper
    get_icc_base_ctx
    hm_submit_cper
    FPFwCoreInterruptIsEnabled
    FPFwCoreInterruptEnableVector
    FPFwCoreInterruptDisableVector
    ddrss_get_telemetry_record
    prod_ddrss_convert_rh_cfg_rec_to_rh_cper
    variable_service_initialize_ctx
    variable_service_sync_get_variable
    variable_service_sync_set_variable
    variable_service_unlock_get_var_ctx
    sdl_get_mem_ctx
    fpfw_init_get_handle
    sos_register_ssi
    DfwkAsyncRequestComplete
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)
