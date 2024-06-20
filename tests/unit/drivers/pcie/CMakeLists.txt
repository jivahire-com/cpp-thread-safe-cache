set(TARGET scp_pcie_driver.test)

set(SOURCES
    pcie_dfwk_mocks.c
    pcie_silibs_mocks.c
    pcie_driver_unit_tests.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.utils
    fwlib.dfwk
    fwlib.platform.dfwkhost.threadx
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::mocks::threadx
    ms::driver::pcie
    silibs::atu
    silibs::idsw
    silibs::kng_silibs_platform
    silibs::pcie
    silibs::pciess
    silibs::pcr_rpss
    silibs::tower_rpss
    silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::driver::pcie,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    # DFWK mocks
    DfwkQueueEnqueueRequest
    DfwkAsyncRequestComplete
    # Threadx mocks
    _txe_timer_create
    _txe_timer_delete
    # Silibs mocks begin
    atu_map
    idsw_get_platform_sdv
    pciess_get_entity
    pciess_config_entity
    pciess_config_ss_for_bifur
    pciess_deassert_por_reset
    pciess_rp_initiate_link_training
    pciess_phys_sram_init_done
    pciess_rps_pre_rp_ready_init
    pciess_rps_ready
    pciess_rps_post_rp_ready_init
)
