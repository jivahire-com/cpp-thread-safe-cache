# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET warm_start_init.test)

set(WARM_START_SOURCE_DIR
    ${CMAKE_SOURCE_DIR}/src/libs/warm_start
)

set(SOURCES
    test_warm_start_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/warm_start_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

target_include_directories(${TARGET}
    PRIVATE
        ${WARM_START_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        ms::lib::boot_status
        ms::lib::warm_start
        ms::cmocka::testrunner
        silibs::idhw
        silibs::kng_silibs_platform
        silibs::idsw
        silibs::idsw_kng

)

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    warm_start_init
    boot_status_notify_extd
    idsw_get_cpu_type
    idsw_get_die_id
)
