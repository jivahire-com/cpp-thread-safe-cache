set(TARGET scp_avs_test.test)

# Compile test
add_executable(${TARGET} scp_avs_test.cpp
                         scp_avs_test_mocks.cpp) 

target_link_libraries(${TARGET}    
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::scp_avs
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
    PUBLIC
        fwlib.assert
        fwlib.dfwk
        fwlib.datastructures.linkedlist
        fwlib.utils
 )

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::lib::scp_avs,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    DfwkInterfaceSendAsync
    DfwkAsyncRequestSetCompletionRoutine    
)

# Supressing warnings, needed for scp_avs.h
target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)
