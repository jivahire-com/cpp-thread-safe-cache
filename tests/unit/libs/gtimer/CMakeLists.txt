# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET gtimer.test)

set(SOURCES
    test_gtimer.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.status
        fwlib.lib.tmr_queue
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::interrupts
        ms::lib::gtimer
        ms::lib::register_headers
        silibs::d2d_counter_sync
        silibs::idsw_kng
        silibs::spi_bridge
        kng_silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    idsw_get_cpu_type
    idhw_get_die_id
    idhw_is_single_die_boot_en
    system_counter_init
    gtimer_init_with_freq
    gtimer_enable_timer
    gtimer_set_timer
    gtimer_get_counter
    gtimer_get_frequency
    FPFwCoreInterruptEnableVector
    FPFwCoreInterruptDisableVector
    FPFwCoreInterruptRegisterCallback
    fpfw_tmr_queue_add_oneshot
    fpfw_tmr_queue_add_periodic
    fpfw_tmr_queue_expire
    mscp_exp_spi_synchronize_dies
)