set(TARGET i3c_controller.test)

set(SOURCES
    test_i3c_controller.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.lib.interface.platform.interrupts
        idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::lib_i3c
        silibs::ddr_i3c
        kng_silibs_platform
        ms::cmocka::testrunner
        ms::lib::i3c_controller
        ms::lib::cortex_m7::nvic
        ms::lib::scp_interrupts
        silibs_platform
)

# @todo remove after completing task https://azurecsi.visualstudio.com/Dev/_workitems/edit/1933312/
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:fwlib.service.iccBase,INCLUDE_DIRECTORIES>)

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "ms::lib::threadx")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    i3c_initialize
    i3c_register_notification_callback
    i3c_master_dat_config
    i3c_master_set_aasa
    i3c_master_register_ibi_handler
    nvic_irq_set_isr_with_param
    nvic_irq_clear_pending
    nvic_irq_enable
)
