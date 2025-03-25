set(TARGET accel_virt_intr.test)

set(SOURCES
    test_accel_virt_intr.cpp
    nvic_mock.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/virt_irq_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        atu
        fwlib.lib.init
        fwlib.lib.interface.platform.interrupts
        fwlib.lib.utils
        kng_silibs_platform
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::accel_intr
        ms::lib::accelerator_ip
        ms::lib::cortex_m7::nvic
        ms::lib::cortex_m7::virt_irq
        ms::lib::register_headers
        ms::lib::scp_interrupts
        ms::lib::utils
        silibs::pcr_rpss
        silibs::sdm_init
        silibs::smmu
        silibs::tower_vab
        silibs::idsw
        silibs::idsw_kng
)

# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::lib::accel_intr,INCLUDE_DIRECTORIES>
        inc/
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
    HW_INT_SDM_NVIC_INT=119
    HW_INT_CDED_SDM_NVIC_INT=118
    HW_INT_SDM_COMB_INT=HW_INT_SDM_NVIC_INT
    HW_INT_CDED_COMB_INT=HW_INT_CDED_SDM_NVIC_INT
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    accelerator_ip_get_atu_mapped_cfg_address
    atu_svc_accel_atu_addr
    mmio_read32
    nvic_irq_set_isr_with_param
    nvic_irq_clear_pending
    nvic_irq_enable
    nvic_irq_disable
    sdm_ext_get_category_mask_reg_addr
    sdm_ext_get_category_status_reg_addr
    sdm_ext_int_mask_enable
    sdm_ext_int_mask_disable
    sdm_ext_int_mask_status_clear
)
