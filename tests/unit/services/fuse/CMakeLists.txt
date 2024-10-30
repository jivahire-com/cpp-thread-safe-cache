set(TARGET fuse.test)

add_executable(${TARGET}
   test_fuse.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::scp_interrupts
        ms::lib::system_info
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::fuse
        ms::service::sds_service
    PUBLIC
        fwlib.lib.assert
        silibs::fuse
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::platform

 )



target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::service::fuse,INCLUDE_DIRECTORIES>)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})


mock_target(${TARGET}
    crash_dump_bug_check
    idsw_get_platform_sdv
    idsw_get_die_id
    fuse_dma_copy_to_ram_blocking
    read_fuse
    apply_fuse_override
    distribute_fuses
    trigger_debugger_for_manual_overrides
    apply_fuse_override_ignoring_valids
    fuse_dist_get_exclusion_list
    fuse_get_exclude_list_fpga
    fpfw_init_get_handle
    fpfw_icc_base_send_recv_sync
    sds_block_creation
    sds_block_write
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)
