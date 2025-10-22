set(TARGET atu_init.test)

set(SOURCES
    test_atu_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/atu_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
     fwlib.dfwk
        fwlib.lib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.linkedlist
        fwlib.lib.utils
        atu
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        ms::cmocka::testrunner
        ms::lib::css
        silibs_platform
        ms::service::atu_service
        ms::lib::threadx
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_init_get_handle
    idhw_is_single_die_boot_en
    idsw_get_die_id
    atu_init
    atu_map
)
