# Copyright (c) Microsoft Corporation. All rights reserved.

# Function to create SOS init test targets
function(create_sos_init_test TARGET_NAME RUNTIME_INIT_DEFINE)
    set(SOURCES
        test_startup_shutdown_init.cpp
        ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/startup_shutdown_init.c
    )

    # Compile test
    add_executable(${TARGET_NAME} ${SOURCES})

    target_link_libraries(${TARGET_NAME}
        PRIVATE
            fwlib.dfwk
            fwlib.lib.init
            fwlib.lib.interface.platform.dbgprint
            fwlib.lib.utils
            fwlib.platform.dfwkhost.threadx
            ms::client_interface::startup_shutdown
            ms::cmocka::testrunner
            ms::lib::boot_status
            ms::lib::crash_dump
            ms::lib::ift
            ms::lib::threadx
            ms::service::scp_startup_shutdown
            ms::lib::hsp_firmware_headers
            ms::driver::icc_mhu_trans
            ms::mocks::event_trace
            kng.lib.hsp_firmware_headers
            silibs::atu
            silibs::idsw_kng
            silibs::kng_silibs_platform
            silibs::idhw
            silibs::idsw
    )

    target_compile_definitions(${TARGET_NAME} PRIVATE ${RUNTIME_INIT_DEFINE})

    # Generic ports implemented within this project
    set_target_properties(${TARGET_NAME} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
    set_target_properties(${TARGET_NAME} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

    lint_target(${TARGET_NAME})
    format_target(${TARGET_NAME})
    coverage_target(${TARGET_NAME})
    unit_test_target(${TARGET_NAME})

    # Common mocks for both targets
    set(COMMON_MOCKS
        DfwkAsyncRequestInitialize
        fpfw_init_get_handle
        sos_init
        sos_interface_init
        sos_register_ssi
        sos_start_phase
        boot_status_notify_extd
        idsw_get_die_id
    )

    # Runtime-specific mocks
    if(${RUNTIME_INIT_DEFINE} STREQUAL "SCP_RUNTIME_INIT")
        mock_target(${TARGET_NAME}
            ${COMMON_MOCKS}
            FpFwLockInitialize
            fpfw_icc_base_recv
            ift_is_enabled
        )
    elseif(${RUNTIME_INIT_DEFINE} STREQUAL "MCP_RUNTIME_INIT")
        mock_target(${TARGET_NAME}
            ${COMMON_MOCKS}
            mcp_sos_icc_init
        )
    endif()
endfunction()

# Create SCP test target
create_sos_init_test(sos_init.test SCP_RUNTIME_INIT)

# Create MCP test target
create_sos_init_test(sos_init_mcp.test MCP_RUNTIME_INIT)