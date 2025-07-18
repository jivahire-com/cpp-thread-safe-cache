set(TARGET power.test.interrupt)
 
# Compile test
add_executable(${TARGET}
    power_telem_interrupt_handler.cpp
)
 
target_sources(power.test.interrupt PRIVATE
    ${CMAKE_SOURCE_DIR}/src/services/power/src/power_interrupt_handler.c
)
 
target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::scp_avs
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::power
        ms::lib::scp_interrupts
        ms::lib::cortex_m7::nvic
 
    PUBLIC
        fwlib.lib.assert
        fwlib.dfwk
        fwlib.lib.linkedlist
        fwlib.lib.utils
)
 
target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)
 
# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::service::power,INCLUDE_DIRECTORIES>
        ../
        ../mocks/
 
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
 
#mock the functions needed
mock_target(${TARGET}
    mmio_read32
    mmio_write32
    core_pll_error_status
    power_loops_control_handle_event
    nvic_get_current_irq
    power_runconfig_get
)
 
target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)
 