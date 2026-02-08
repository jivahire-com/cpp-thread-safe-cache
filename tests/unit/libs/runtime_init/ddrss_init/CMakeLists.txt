# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET ddrss_init.test)

set(SOURCES
    test_ddrss_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/ddrss_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

if (i386-pc-windows-msvc)
    set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
    set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
endif()

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        idhw
        ms::cmocka::testrunner
        ms::lib::ddr_manager
        ms::lib::ddrss_init
        ms::lib::system_info
        silibs_platform
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

#mock the functions needed
mock_target(${TARGET}
    FPFwCoreInterruptEnableVector
    FPFwCoreInterruptRegisterCallback
    ddr_manager_init
    ddrss_atu_map_cfg_space
    ddrss_atu_unmap_cfg_space
    ddrss_set_die_base
    idhw_get_die_id
    idsw_get_die_id
    idhw_is_single_die_boot_en
    pcr_ddrss_configure_clock_and_pcr_reset
    system_info_is_warm_start
    config_get_fips_kat_en
    config_get_pas_encryption_en_mask
    idsw_get_platform_sdv
    fpfw_init_get_handle
    fpfw_icc_base_send_recv_sync
    fpfw_icc_base_send_sync
)
