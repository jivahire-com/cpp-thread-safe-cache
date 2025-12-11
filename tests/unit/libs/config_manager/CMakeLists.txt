set(TARGET config_manager.test)

set(SOURCES
    config_manager_ut.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::lib::config_manager,INCLUDE_DIRECTORIES>
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.lib.status
        fwlib.lib.spinlock
        silibs::idsw_kng
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::lib::error_codes
        ms::lib::crash_dump
        ms::lib::config_manager
        ms::lib::config_manager::cli
        ms::lib::variable_services
        fwlib.lib.interface.platform.dbgprint
)

# Provide compile time definition
target_compile_definitions(
    ${TARGET}
    PRIVATE
        SCP_RUNTIME_INIT
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_cfg_mgr_get_cached_knob_values_size
    fpfw_cfg_mgr_get_cached_knob_values
    fpfw_cfg_mgr_set_cached_knob_values
    system_info_is_hsp_present
    variable_service_initialize_ctx
    variable_service_sync_get_variable
    variable_service_async_set_variable
    variable_service_async_get_variable
    variable_service_unlock_get_var_ctx
    update_knob_in_cached_db_cb
    idsw_get_die_id
    idhw_is_single_die_boot_en
    write_override_knob_to_shared_rmss
    mscp_exp_spi_synchronize_dies_timeout
    apply_override_knob_from_primary_die
    spi_controller_read_direct_instruction
    idsw_get_cpu_type
    idsw_get_platform_sdv
    system_info_get_mission_mode
)
