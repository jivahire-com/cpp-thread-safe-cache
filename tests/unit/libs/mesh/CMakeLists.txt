set(TARGET mesh.test)

set(SOURCES
    test_mesh.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        cmn800
        cmn800_sequence
        cmn800_ras_sequence
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.lib.interface.platform.interrupts
        idhw
        silibs::idsw
        silibs::idsw_kng
        kng_silibs_platform
        ms::cmocka::testrunner
        ms::lib::mesh
        ms::lib::cortex_m7::nvic
        ms::lib::scp_interrupts
        silibs_platform
        ms::lib::hsp_firmware_headers
        fwlib.service.iccBase
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
    cmn800_sequence
    idhw_is_single_die_boot_en
    idhw_get_die_id
    nvic_irq_set_isr_with_param
    nvic_irq_clear_pending
    nvic_irq_enable
    d2dss_sequence
    cmn800_sequence_svp_updates
    fpfw_icc_base_send_recv_sync
    system_info_is_hsp_present
    interrupt_handler_mesh_ras_error
)
