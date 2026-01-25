# Copyright (c) Microsoft Corporation. All rights reserved.

#
# SCP Crash Dump init test
#
set(TARGET crash_dump_init.scp.test)

set(SOURCES
    test_crash_dump_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/scp_crash_dump_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(${TARGET}
    PRIVATE
        ${CRASH_DUMP_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.crashDump
        fwlib.platform.dfwkhost.threadx
        fwlib.service.iccBase
        kng.lib.hsp_firmware_headers
        ms::client_interface::startup_shutdown
        ms::cmocka::testrunner
        ms::cmsis::interface
        ms::lib::accelerator_ip
        ms::lib::boot_status
        ms::lib::crash_dump
        ms::lib::exception_handler::scp
        ms::lib::system_info
        silibs::icc_mhu
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::platform
        silibs::kng_silibs_platform
        silibs::sdm_command_desc
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    CdRegisterMMIORegisterSet
    accel_is_isolation_enabled
    atu_svc_accel_atu_addr
    crash_dump_device_initialize
    crash_dump_interface_initialize
    sos_register_ssi
    crash_dump_context
    crash_dump_init
    crash_dump_register_dump
    crash_dump_config_icc
    exception_handler_init
    idhw_is_single_die_boot_en
    idsw_get_die_id
    idsw_get_cpu_type
    idsw_get_platform_sdv
    boot_status_notify_extd
)

#
# MCP Crash Dump init test
#
set(TARGET crash_dump_init.mcp.test)

set(SOURCES
    test_crash_dump_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/mcp_crash_dump_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(${TARGET}
    PRIVATE
        ${CRASH_DUMP_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.crashDump
        fwlib.platform.dfwkhost.threadx
        fwlib.service.pldm
        kng.lib.hsp_firmware_headers
        ms::client_interface::startup_shutdown
        ms::cmocka::testrunner
        ms::cmsis::interface
        ms::driver::gpio
        ms::driver::pldm    # PLDM_DRV_WORKAROUND
        ms::lib::boot_status
        ms::lib::crash_dump
        ms::lib::system_info
        ms::lib::exception_handler::mcp
        silibs::icc_mhu
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::platform
        silibs::kng_silibs_platform
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       MCP_RUNTIME_INIT
       PLDM_DRV_WORKAROUND
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    DfwkClientInterfaceOpen
    DfwkAsyncRequestSetCompletionRoutine    # PLDM_DRV_WORKAROUND
    crash_dump_bug_check
    crash_dump_init
    crash_dump_register_dump
    crash_dump_config_icc
    crash_dump_device_initialize
    crash_dump_interface_initialize
    crash_dump_oob_transfer
    exception_handler_init
    pldm_drv_register_platform_event_ready_notification # PLDM_DRV_WORKAROUND
    # fpfw_pldm_service_register_platform_event_ready_notification # !PLDM_DRV_WORKAROUND
    gpio_interface_init
    gpio_register_deferred_isr
    idsw_get_die_id
    idsw_get_cpu_type
    idsw_get_platform_sdv
    crash_dump_transfer_full_dump_to_bmc
    sos_register_ssi
    _txe_timer_create
    boot_status_notify_extd
)