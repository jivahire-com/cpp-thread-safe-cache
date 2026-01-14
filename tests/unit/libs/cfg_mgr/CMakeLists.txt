# Copyright (c) Microsoft Corporation. All rights reserved.

#
# config manager unit test
#
set(TARGET cfg_mgr.test)

set(SOURCES
    test_cfg_mgr.cpp
    # ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/cfg_mgr_init.c
    # test_cfg_mgr_cli.cpp
    # test_cfg_mgr_mocks.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/config_manager/src/config_manager.c
    ${CMAKE_SOURCE_DIR}/src/libs/config_manager/src/config_manager_hsp.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:fwlib.lib.cfg_mgr,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:ms::lib::config_manager,INCLUDE_DIRECTORIES>
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.assert
    fwlib.lib.init
    fwlib.service.cli
    ms::build_data
    ms::cmocka::testrunner
    ms::cmsis::interface
    ms::lib::crash_dump
    ms::lib::threadx
    silibs::kng_silibs_platform
    silibs_platform
    ms::lib::variable_services
    ms::lib::config_manager
    ms::lib::config_manager::cli
    fwlib.lib.spinlock
    ms::lib::error_codes
    ms::lib::crash_dump
    ms::lib::system_info
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    # empty_read_knob_fn
    # empty_write_knob_fn
)
