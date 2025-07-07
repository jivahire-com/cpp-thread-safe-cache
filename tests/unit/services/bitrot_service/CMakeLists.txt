set(TARGET bitrot_service.test)

# Compile test
add_executable(${TARGET}
    bitrot_service_test.cpp
    ${CMAKE_SOURCE_DIR}/src/services/bitrot_service/src/bitrot_service.c
)

target_link_libraries(${TARGET}
    PRIVATE
    ms::cmocka::testrunner
    ms::mocks::event_trace
)

# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
    $<TARGET_PROPERTY:kng.service.bitrot_service,INCLUDE_DIRECTORIES>
)

# Defining platform APIs
lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# mock the functions needed
mock_target(${TARGET}
    tx_thread_create
    tx_thread_sleep
    config_get_bitrot_TCM_delay
    FpFwCliPrint
    FPFwErrorRaise
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)
