set(TARGET gtimer.test)

set(SOURCES
    test_gtimer.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::gtimer
        ms::lib::register_headers
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

