set(TARGET wdog_init.test)

set(SOURCES
    test_watchdog_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/watchdog_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::service::wdog
        ms::lib::config_manager
        ms::lib::threadx
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::platform
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    _txe_timer_create
    wdog_cmsdk_apb_init
    wdog_cmsdk_apb_reload
)
