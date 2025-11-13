set(TARGET ift.test)

set(SOURCES
    test_ift.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        silibs::idsw
        silibs::idsw_kng
        silibs::sdm_command_desc
        ms::client_interface::startup_shutdown
        ms::lib::cortex_m7::mpu
        ms::lib::cortex_m7::nvic
        ms::lib::hsp_firmware_headers
        ms::lib::ift
        ms::lib::register_headers
        ms::lib::scp_interrupts
        ms::lib::system_info
        ms::lib::utils
        ms::cmocka::testrunner
        kng_silibs_platform
    PUBLIC
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.status
        fwlib.lib.utils
        ms::lib::fuse_artifacts_lib
        ms::lib::icc_platform_defines
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/libs/ift/src
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    DfwkAsyncRequestComplete
    DfwkDeviceInitialize
    DfwkQueueInitialize
    DfwkInterfaceInitialize
    DfwkClientInterfaceOpen
    DfwkAsyncRequestInitialize
    DfwkAsyncRequestSetCompletionRoutine
    DfwkInterfaceSendAsync
    SCB_EnableDCache
    SCB_EnableICache
    SCB_InvalidateDCache
    SCB_InvalidateICache

    crash_dump_bug_check
    fuse_read_data

    idsw_get_die_id
    ift_execute_ist

    fpfw_icc_base_send_sync
    fpfw_icc_base_recv
    fpfw_icc_base_send_recv_sync
    fpfw_icc_base_send_recv
    fpfw_init_get_handle

    mmio_write32
    mmio_read32
    mpu_init
    nvic_irq_disable

    sleep_us
    sos_start_phase
    spi_controller_bulk_write
    spi_controller_write_direct_instruction
)
