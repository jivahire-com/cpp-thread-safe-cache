# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET scp_avs_test.test)

# Compile test
add_executable(${TARGET} scp_avs_test.cpp
                         scp_avs_test_mocks.cpp)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::scp_interrupts
        ms::lib::scp_avs
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
    PUBLIC
        fwlib.lib.assert
        fwlib.dfwk
        fwlib.lib.linkedlist
        fwlib.lib.utils
        silibs::kng_silibs_platform
 )

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::lib::scp_avs,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    DfwkAsyncRequestSetCompletionRoutine
    DfwkInterfaceSendAsync
    DfwkAsyncRequestComplete
    DfwkAsyncRequestSetCompletionRoutine
    avs_init
    DfwkQueueEnqueueRequest
    avs_clear_interrupt_status
    avs_enable_interrupt
    avs_get_cmd_resp_data
    avs_get_interrupt_status
    avs_init
    avs_send_cmd_frame
    nvic_irq_set_isr_with_param
    nvic_irq_clear_pending
    nvic_irq_enable
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

# Supressing warnings, needed for scp_avs.h
target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers -Wno-bitfield-constant-conversion)

# Suppressing warnings for the bitfield enum assignment.
target_compile_options(scp_avs PRIVATE -Wno-bitfield-constant-conversion)