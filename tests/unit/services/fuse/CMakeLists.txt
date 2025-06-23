set(TARGET fuse.test)

add_executable(${TARGET}
   test_fuse.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::fuse_artifacts_lib
        ms::lib::scp_interrupts
        ms::lib::system_info
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::fuse
        ms::service::sds_service
    PUBLIC
        fwlib.lib.assert
        fwlib.dfwk
        fwlib.lib.cfg_mgr
        fwlib.lib.linkedlist
        fwlib.lib.utils
        silibs::fuse
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::platform

 )


target_include_directories(${TARGET} 
    PRIVATE 
        $<TARGET_PROPERTY:fwlib.lib.cfg_mgr,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:ms::lib::config_manager,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:ms::service::fuse,INCLUDE_DIRECTORIES>
)

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
    fuse_read
    read_core_defect_fuses
    fuse_override
    fuse_distribution
    trigger_debugger_for_manual_overrides
    fuse_override_ignoring_valids
    fuse_dist_get_exclusion_list
    fuse_get_exclude_list_fpga
    fpfw_init_get_handle
    fpfw_icc_base_send_recv_sync
    sds_block_creation
    sds_block_write
    fpfw_icc_base_send
    fpfw_icc_base_recv
    idhw_is_single_die_boot_en
    system_info_is_warm_start
    config_get_core_disable_value_0_31
    config_get_core_disable_value_32_63
    config_get_core_disable_value_64_95
    config_get_core_spare_en_0_31
    config_get_core_spare_en_32_63
    config_get_core_spare_en_64_95
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)
