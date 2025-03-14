set(TARGET mcp_clock_init.test)

set(SOURCES
    test_mcp_clock_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/mcp_clock_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.utils
        ms::cmocka::testrunner
        silibs::pik_clock
        silibs::platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    pik_clock_init
    pik_clock_power_transition
)
