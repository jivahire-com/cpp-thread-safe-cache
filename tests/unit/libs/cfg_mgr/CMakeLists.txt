#
# config manager unit test
#
set(TARGET cfg_mgr.test)

set(SOURCES
    test_cfg_mgr.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/cfg_mgr_init.c
    # test_cfg_mgr_cli.cpp
    # test_cfg_mgr_mocks.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(${TARGET}
    PRIVATE
        ${CRASH_DUMP_SOURCE_DIR}/inc
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:fwlib.lib.cfg_mgr,INCLUDE_DIRECTORIES>)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.assert
    fwlib.lib.init
    fwlib.lib.cfg_mgr
    fwlib.service.cli
    ms::build_data
    ms::cmocka::testrunner
    ms::cmsis::interface
    ms::lib::crash_dump
    ms::lib::threadx
    silibs::kng_silibs_platform
    silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    # empty_read_knob_fn
    # empty_write_knob_fn
)