# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET corebits.test)

set(SOURCES
    test_corebits.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::corebits
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})
