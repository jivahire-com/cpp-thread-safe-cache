set(TARGET tower.test)

set(SOURCES
    tower_mocks.c
    tower_unit_tests.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.service.iccBase
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::error_handler
        ms::lib::hsp_firmware_headers
        ms::lib::tower
        silibs::atu
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::tower_sequence
        silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    tower_sequence_configure_towers
    atu_map
    atu_unmap
    idsw_get_platform_sdv
    system_info_is_hsp_present
    fpfw_icc_base_send_recv_sync
)
