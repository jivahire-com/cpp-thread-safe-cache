set(TARGET power_init.test)

set(SOURCES
    test_power_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/power_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.init
        fwlib.utils
        fwlib.platform.dfwkhost.threadx
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::service::power
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_init_get_handle
    power_init
    power_interface_init
)
