set(TARGET cfg_mgr_init.test)

set(SOURCES
    test_cfg_mgr_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/cfg_mgr_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.linkedlist
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::scp_interrupts
        ms::lib::threadx
        silibs_platform
        silibs::idsw_kng
        silibs::kng_silibs_platform
        ms::lib::config_manager
        ms::lib::config_manager::cli
        ms::lib::variable_services
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    cfg_mgr_init
    cfg_mgr_cli_init
    idsw_get_platform_sdv
)
