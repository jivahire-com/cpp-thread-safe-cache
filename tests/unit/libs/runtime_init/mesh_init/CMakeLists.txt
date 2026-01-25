# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET mesh_init.test)

set(SOURCES
    test_mesh_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/mesh_init.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    cmn800
    cmn800_sequence
    fwlib.lib.init
    fwlib.lib.interface.platform.dbgprint
    fwlib.lib.utils
    idhw
    silibs::idsw
    silibs::idsw_kng
    kng_silibs_platform
    ms::cmocka::testrunner
    ms::lib::mesh
    ms::lib::boot_status
    silibs_platform
    fwlib.service.iccBase
)

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "ms::lib::threadx")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    idhw_get_die_id
    fpfw_init_get_handle
    mesh_init
    d2d_init
    post_led_status
    boot_status_notify_extd
    idsw_get_cpu_type
    idsw_get_die_id
)
