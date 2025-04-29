set(TARGET accelerator_ip.test)

set(SOURCES
    test_accelerator_ip.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        atu
        fwlib.lib.cfg_mgr
        fwlib.lib.interface.platform.timers
        fwlib.service.iccBase
        silibs::idsw
        silibs::idsw_kng
        silibs::sdm_command_desc
        silibs::pcr_rpss
        silibs::sdm_init
        silibs::smmu
        silibs::tower_vab
        ms::lib::accelerator_ip
        ms::lib::boot_status
        ms::lib::hsp_firmware_headers
        ms::lib::system_info
        ms::lib::threadx
        ms::lib::utils
        ms::cmocka::testrunner
        kng_silibs_platform
    PUBLIC
        fwlib.lib.assert
        fwlib.dfwk
        fwlib.lib.linkedlist
        fwlib.lib.status
        fwlib.lib.utils
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
    accel_intr_get_atu_mapped_cfg_address
    accel_mcp_intr_init
    accel_scp_intr_init
    accelip_ss_init
    atu_map
    atu_svc_accel_atu_addr
    atu_unmap
    boot_status_notify_extd
    configure_cdedss_hsp_system_addr_map
    configure_cdedss_vab_system_addr_map
    config_get_cded_isolation_enable
    config_get_sdm_isolation_enable
    crash_dump_bug_check
    critical_print
    debug_print
    FpFwAssert
    fpfw_icc_base_recv
    fpfw_icc_base_send
    fpfw_icc_base_send_recv_sync
    fpfw_icc_base_send_resp
    fpfw_init_get_handle
    fpfw_timer_create
    fpfw_timer_reset
    idsw_get_die_id
    idsw_get_platform_sdv
    mmio_read32
    mmio_update32
    pcr_rpss_configure_clock
    pcr_rpss_deassert_por_reset
    pcr_rpss_init_entity
    post_led_status
    sdm_init_assert_nsysreset
    sdm_init_deassert_nsysreset
    sdm_init_disable_cpu_wait
    sdm_init_enable_cpuwait
    sdm_init_enable_fence
    sdm_init_enable_fence
    sdm_init_itcm_enable
    system_info_is_hsp_present
)
