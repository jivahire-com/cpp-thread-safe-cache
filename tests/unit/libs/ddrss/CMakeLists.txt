set(TARGET ddrss.test)

set(SOURCES
    ddrss_mocks.c
    test_ddrss.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::cortex_m7::nvic
        ms::lib::ddrss_init
        ms::lib::register_headers
        ms::lib::scp_interrupts
        silibs::atu
        silibs::ddrss
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::ras_arm
        fwlib.lib.interface.platform.interrupts
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    atu_map
    atu_unmap
    cmn800_generate_ddr_mc_map_from_cached_config
    ddrss_get_phy_interrupt_status
    ddrss_clear_phy_interrupt_status
    nvic_irq_set_isr_with_param
    nvic_irq_clear_pending
    nvic_irq_enable
    mmio_read32
    mmio_write32
    ddrss_init
    idhw_is_single_die_boot_en
    intu_get_interrupt_status
    ddrss_ddr_intu_get_interrupt_dest_enable
    ddrss_probe_ras_agent
    ddrss_get_phy_interrupt_status
    ddrss_mc_intu_get_interrupt_dest_enable
    ddrss_mc_event_clear_interrupt
    ddrss_ddr_intu_clear_interrupt
    ddrss_ddr_intu_clear_destination_interrupt
    ras_arm_agent_probe
    ddrss_get_ras_agent
    ddrss_mc_intu_get_interrupt_status
)
