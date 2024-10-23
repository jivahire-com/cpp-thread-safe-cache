set(TARGET warm_start_cli_init.test)

set(WARM_START_CLI_SOURCE_DIR
    ${CMAKE_SOURCE_DIR}/src/libs/warm_start_cli
)

set(SOURCES
    test_warm_start_cli_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/warm_start_cli_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

target_include_directories(${TARGET}
    PRIVATE
        ${WARM_START_CLI_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        ms::lib::warm_start::cli
        ms::cmocka::testrunner
        ms::service::scp_startup_shutdown
        fwlib.dfwk
)

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    warm_start_cli_init
)
