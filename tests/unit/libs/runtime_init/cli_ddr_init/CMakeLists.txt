set(TARGET cli_ddr_init.test)

set(SOURCES
    test_cli_ddr_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/cli_ddr_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.service.cli
        ms::lib::ddr::cli
        ms::cmocka::testrunner
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    cli_ddr_init
)
