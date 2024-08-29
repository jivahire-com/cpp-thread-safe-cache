set(TARGET vab_driver.test)

set(SOURCES
    vab_mocks.c
    vab_unit_tests.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.utils
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::atomics
    ms::lib::cortex_m7::nvic
    ms::lib::error_handler
    ms::lib::interrupts
    ms::lib::scp_interrupts
    ms::driver::pcie
    ms::driver::vab
    silibs::atu
    silibs::idsw
    silibs::idsw_kng
    silibs::kng_silibs_platform
    silibs_platform
    silibs::vab_init
    silibs::smmu
    silibs::intu
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    atu_map
    atu_unmap
    idsw_get_platform_sdv
    intu_clear_interrupt_status
    intu_get_interrupt_status
    nvic_irq_set_isr_with_param
    nvic_irq_clear_pending
    nvic_irq_enable
    rpss_irq_callback
    vab_init
)
