set(TARGET ap_core.test)

# Compile test
add_executable(${TARGET}
    ap_core_main_test.cpp
    ap_core_ppu_test.cpp
    ap_core_util_test.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.service.iccBase
        ms::cmocka::testrunner
        ms::lib::hsp_firmware_headers 
        ms::lib::scp_avs
        ms::lib::system_info
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::ap_core
        ms::service::sds_service
    PUBLIC
        fwlib.lib.assert
        fwlib.dfwk
        fwlib.lib.linkedlist
        fwlib.lib.utils
 )

# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::service::ap_core,INCLUDE_DIRECTORIES>
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
    FpFwAssert
    DfwkAsyncRequestComplete
    DfwkDeviceInitialize
    DfwkQueueInitialize
    DfwkInterfaceInitialize
        
    ap_core_ppu_clusters_on
    ap_core_ppu_core_set_power_state
    ap_core_ppu_init

    ap_core_util_boot_core
    ap_core_util_get_fuse_enabled_cores
    ap_core_util_set_all_rvbaraddr
    ap_core_util_set_rvbaraddr

    fpfw_icc_base_send
    fpfw_icc_base_recv

    mmio_read32
    sds_block_creation
    sds_block_write

    ppu_v1_init

    system_info_is_hsp_present
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)