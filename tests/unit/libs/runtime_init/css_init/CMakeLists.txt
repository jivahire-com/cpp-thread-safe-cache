# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET css_init.test)

set(SOURCES
    test_css_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/css_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        atu
        clocks_sequence
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        idhw
        silibs::idsw
        silibs::idsw_kng
        ms::cmocka::testrunner
        ms::lib::css
        silibs_platform
        tower_system_control
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    css_configure_system_tower
    css_post_mesh_init
    css_pre_mesh_init
    idhw_get_die_id
    idsw_get_platform_sdv
)
