set(TARGET extern.fpfw.port.atomics.test)

# Compile test
add_executable(
    ${TARGET} 
        atomics_test.cpp
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        ms::lib::atomics
        ms::cmocka::testrunner
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})
