# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET vab_init.test)

set(SOURCES
    test_vab_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/vab_init.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.init
    fwlib.lib.interface.platform.dbgprint
    ms::cmocka::testrunner
    ms::driver::vab
    ms::service::health_monitor
    silibs::idsw
    silibs::idsw_kng
    silibs::kng_silibs_platform
    silibs::vab_init
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    enable_vab_isrs
    idsw_get_platform_sdv
    idsw_get_die_id
    vab_common_init
)
