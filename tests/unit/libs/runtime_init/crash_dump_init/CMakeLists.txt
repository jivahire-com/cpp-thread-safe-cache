#
# SCP Crash Dump init test
#
set(TARGET crash_dump_init.scp.test)

set(SOURCES
    test_crash_dump_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/crash_dump_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(${TARGET}
    PRIVATE
        ${CRASH_DUMP_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.crashDump
        ms::cmocka::testrunner
        ms::cmsis::interface
        ms::lib::crash_dump
        silibs::icc_mhu
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::platform
        silibs::kng_silibs_platform
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    crash_dump_init
    crash_dump_init_post_mesh
    idsw_get_die_id
    idsw_get_cpu_type
    idsw_get_platform_sdv
)

#
# MCP Crash Dump init test
#
set(TARGET crash_dump_init.mcp.test)

set(SOURCES
    test_crash_dump_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/crash_dump_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(${TARGET}
    PRIVATE
        ${CRASH_DUMP_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.crashDump
        ms::cmocka::testrunner
        ms::cmsis::interface
        ms::lib::crash_dump
        silibs::icc_mhu
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::platform
        silibs::kng_silibs_platform
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       MCP_RUNTIME_INIT
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    crash_dump_init
    crash_dump_init_post_mesh
    idsw_get_die_id
    idsw_get_cpu_type
    idsw_get_platform_sdv
)