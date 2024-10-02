set(TARGET power.test.cli)

# Compile test
add_executable(${TARGET}
    power_cli_requests_tests.cpp    
    ../mocks/mock_bug_check.c
)



target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::scp_avs
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::power
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

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
   DfwkAsyncRequestComplete
   power_cap_update   
   power_loops_exec_in_idle
   power_runconfig_get
)


target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)