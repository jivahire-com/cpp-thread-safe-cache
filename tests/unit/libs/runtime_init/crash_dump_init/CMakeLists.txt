set(TARGET crash_dump_init.test)

set(CRASH_DUMP_SOURCE_DIR
    ${CMAKE_SOURCE_DIR}/src/libs/crash_dump
)

set(CRASH_DUMP_SOURCES
    ${CRASH_DUMP_SOURCE_DIR}/src/crash_dump.c
    ${CRASH_DUMP_SOURCE_DIR}/src/crash_dump_init.c
    ${CRASH_DUMP_SOURCE_DIR}/src/crash_dump_overrides.c
    ${CRASH_DUMP_SOURCE_DIR}/src/crash_dump_payload_core.c
    ${CRASH_DUMP_SOURCE_DIR}/src/crash_dump_payload_msft.c
    ${CRASH_DUMP_SOURCE_DIR}/src/crash_dump_payload_threadx.c
    ${CRASH_DUMP_SOURCE_DIR}/src/scp/crash_dump_init_scp.c
    ${CRASH_DUMP_SOURCE_DIR}/src/scp/crash_dump_gpio.c
    ${CRASH_DUMP_SOURCE_DIR}/src/scp/crash_dump_overrides.c
    ${CRASH_DUMP_SOURCE_DIR}/src/scp/crash_dump_payload.c
)

set(SOURCES
    test_crash_dump_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/crash_dump_init.c
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/crash_dump_init_scp.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

target_include_directories(${TARGET}
    PRIVATE
        ${CRASH_DUMP_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.crashDump
        ms::cmocka::testrunner
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    crash_dump_init
    crash_dump_init_post_mesh
)
