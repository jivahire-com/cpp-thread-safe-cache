# Copyright (c) Microsoft Corporation. All rights reserved.

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

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
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
    fuse_dist_get_exclusion_list_secure
    fuse_get_exclude_list_fpga
    fpfw_init_get_handle
    fpfw_icc_base_send_recv_sync
    sds_block_creation
    sds_block_write
    system_info_get_security_state
    fpfw_icc_base_send
    fpfw_icc_base_recv
    idhw_is_single_die_boot_en
    system_info_is_warm_start
    config_get_die0_core_disable_value_31_0
    config_get_die0_core_disable_value_63_32
    config_get_die0_core_disable_value_95_64
    config_get_die1_core_disable_value_31_0
    config_get_die1_core_disable_value_63_32
    config_get_die1_core_disable_value_95_64
    config_get_die0_core_spare_en_31_0
    config_get_die0_core_spare_en_63_32
    config_get_die0_core_spare_en_95_64
    config_get_die1_core_spare_en_31_0
    config_get_die1_core_spare_en_63_32
    config_get_die1_core_spare_en_95_64
    mesh_get_hns_sds_vector_from_hns_sparring
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)
