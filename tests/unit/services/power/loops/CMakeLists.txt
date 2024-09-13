set(TARGET power.test.loops)

# Compile test
add_executable(${TARGET}
    power_loops_control_test.cpp    
    power_loops_telemetry_test.cpp   
    power_loops_test.cpp
    power_remote_die_test.cpp
    
    ../mocks/mock_dvfs.c
    ../mocks/mock_odcm.c
    ../mocks/mock_pvt.c
    ../mocks/mock_bug_check.c
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::scp_avs
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::power
        ms::service::sensor_fifo
    PUBLIC
        fwlib.lib.assert
        fwlib.dfwk
        fwlib.lib.linkedlist
        fwlib.lib.utils
 )

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::service::power,INCLUDE_DIRECTORIES>
        ../
        ../mocks/

)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    _txe_queue_create
    _txe_queue_front_send
    _txe_queue_info_get
    _txe_queue_receive
    _txe_queue_send
    _txe_thread_create
    FpFwAssert
    corebits_first
    cortex_m7_atomic_or
    fpfw_icc_base_recv
    fpfw_icc_base_send
    pid_calculate_resources
    pid_init
    pid_reset
    pid_set_resources
    pid_update_setpoint
    power_cap_get_vrcpu_cap
    power_cap_finalize
    power_distribution_distribute_resources
    power_distribution_distribute_warmstart_resources
    power_distribution_get_minimum_plimit
    power_distribution_minimum_plimit_updates
    power_hw_clear_force_pmin
    power_hw_force_pmin
    power_hw_gpio_rack_limit_asserted
    power_loops_init_loop
    power_loops_change_state
    power_loops_handle_event
    power_loops_retry_fail
    power_remote_die_exchange_inputs
    power_remote_die_exchange_complete
    power_remote_die_idle_reset
    power_remote_die_init
    power_runconfig_get
    power_set_plimit
    power_telemetry_message_poll
    power_timer_get_counter
    power_timer_get_counter_ticks_us
    power_timer_get_us_from_counter
    power_vrs_initiate_vr_reads
    power_vrs_read_vcpu_voltage
    power_vrs_write_vcpu_voltage
    scf_get_core_state
    soc_pvt_handle_dts_irq_exclear
    soc_pvt_handle_vm_irq_exclear
    sensor_fifo_svc_add_soc_pvt_temperature
    sensor_fifo_svc_add_soc_pvt_voltage
    sensor_fifo_svc_add_vr_temperature
    sensor_fifo_svc_add_vr_current
    power_vcpu_calc_max_core_voltage_mv
    power_vcpu_calc_peak_current_A
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)