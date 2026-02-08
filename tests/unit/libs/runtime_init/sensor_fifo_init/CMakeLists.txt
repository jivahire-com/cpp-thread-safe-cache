# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET sensor_fifo_init.scp.test)

set(SOURCES
    test_sensor_fifo_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/sensor_fifo_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        fwlib.service.cli
        fwlib.service.iccBase
        fwlib.service.pldm
        ms::cmocka::testrunner
        ms::lib::boot_status
        ms::lib::error_codes
        ms::lib::system_info
        ms::lib::threadx
        silibs::idhw
        ms::driver::sensor_fifo
        ms::lib::core_info
        ms::service::sensor_fifo
        ms::service::sensor_fifo::cli
        ms::service::scp_startup_shutdown
        silibs::kng_silibs_platform
        silibs::sensor_ram_bridge
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")


lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    idsw_get_die_id
    idsw_get_cpu_type
    boot_status_notify_extd
    sensor_fifo_cli_svc_initialize
    sensor_fifo_svc_initialize
    sensor_fifo_driver_inf_init 
    scf_mhu_device_initialize 
    fpfw_init_get_handle
    core_info_get_enable_cores_result
    idsw_get_platform_sdv
)

set(TARGET sensor_fifo_init.mcp.test)

set(SOURCES
    test_sensor_fifo_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/sensor_fifo_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_compile_definitions(
    ${TARGET}
    PRIVATE
    MCP_RUNTIME_INIT
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        fwlib.service.cli
        fwlib.service.iccBase
        fwlib.service.pldm
        ms::cmocka::testrunner
        ms::driver::pldm    # PLDM_DRV_WORKAROUND
        ms::lib::boot_status
        ms::lib::threadx
        silibs::icc_mhu
        silibs::idhw
        ms::driver::sensor_fifo
        ms::lib::core_info
        ms::service::sensor_fifo
        ms::service::sensor_fifo::cli
        ms::service::mcp_startup_shutdown
        silibs::kng_silibs_platform
        silibs::sensor_ram_bridge
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")


lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET} 
    idsw_get_die_id
    idsw_get_cpu_type
    boot_status_notify_extd
    sensor_fifo_cli_svc_initialize
    sensor_fifo_svc_initialize
    sensor_fifo_driver_inf_init 
    scf_mhu_device_initialize 
    fpfw_init_get_handle
    core_info_get_enable_cores_result
    idsw_get_platform_sdv
    )
