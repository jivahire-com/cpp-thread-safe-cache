# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET tower.test)

set(SOURCES
    tower_mocks.c
    tower_unit_tests.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.service.iccBase
        fwlib.lib.cfg_mgr
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::accelerator_ip
        ms::lib::error_handler
        ms::lib::hsp_firmware_headers
        ms::lib::tower
        ms::service::atu_service
        ms::service::health_monitor
        silibs::atu
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::ras_framework
        silibs_platform
        silibs::silibs_utils
        silibs::tower_sequence
        silibs::tower_utility
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    accel_is_isolation_enabled
    atu_map
    atu_unmap
    config_get_tower_knobs
    crash_dump_bug_check
    fpfw_icc_base_send_recv_sync
    idhw_is_single_die_boot_en
    idsw_get_die_id
    idsw_get_platform_sdv
    ras_agent_probe
    ras_agent_record_to_cper
    ras_arm_fmu_agent_set_base
    ras_print_record
    system_info_is_hsp_present
    tower_fmu_inject_single_error
    ras_arm_fmu_agent_trigger_by_type
    tower_get_ras_agent_entity
    tower_sequence_configure_towers
)
