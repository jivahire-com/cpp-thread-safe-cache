set(TARGET accel_intr.test)

set(SOURCES
    fpfw_mock.cpp
    nvic_mock.cpp
    sdm_ext_interrupts_mock.cpp
    silibs_platform_mock.cpp
    test_accel_intr.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        atu
        fwlib.lib.init
        fwlib.lib.interface.platform.interrupts
        fwlib.lib.utils
        kng_silibs_platform
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::accel_intr
        ms::lib::accelerator_ip
        ms::lib::cortex_m7::nvic
        ms::lib::register_headers
        ms::lib::scp_interrupts
        ms::lib::utils
        silibs::pcr_rpss
        silibs::sdm_init
        silibs::smmu
        silibs::tower_vab
        silibs::idsw
        silibs::idsw_kng
)

# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::lib::accel_intr,INCLUDE_DIRECTORIES>
        inc/
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

# Mock the functions needed
mock_target(${TARGET}
    accel_intr_service_get_cfg_address
    accel_mbox_sw_intr_cb
    fpfw_timer_create
    fpfw_timer_enable
    fpfw_timer_reset
    idsw_get_cpu_type
    idsw_get_platform_sdv
    is_sdm_ext_int_mask_status_clear
    is_sdm_ext_int_status_set
    mmio_read32
    mmio_update32
    mmio_write32
    nvic_irq_clear_pending
    nvic_irq_disable
    nvic_irq_enable
    nvic_irq_set_isr_with_param
    sdm_ext_get_category_mask_reg_addr
    sdm_ext_get_category_status_reg_addr
    sdm_ext_int_mask_disable
    sdm_ext_int_mask_enable
    sdm_ext_int_mask_status_clear
    send_fatal_intr_async_request
    send_mailbox_async_request
    send_sdm_msg_async_request
    set_ext_int_sub_system
)
