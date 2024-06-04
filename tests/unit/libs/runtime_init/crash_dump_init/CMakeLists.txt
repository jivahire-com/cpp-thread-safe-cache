set(TARGET crash_dump_init.test)

set(SOURCES
    test_crash_dump_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/crash_dump_init.c
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/crash_dump_init_scp.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.init
        fwlib.crashDump
        ms::cmocka::testrunner
        ms::lib::crash_dump::scp
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
