set(TARGET power.test.runconfig)

# Compile test
add_executable(${TARGET} 
    power_runconfig_test.cpp
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
        fwlib.assert
        fwlib.dfwk
        fwlib.datastructures.linkedlist
        fwlib.utils
 )

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
    dvfs_vft_from_fuse_data    
    power_fuses_read    
    power_knobs_read
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)