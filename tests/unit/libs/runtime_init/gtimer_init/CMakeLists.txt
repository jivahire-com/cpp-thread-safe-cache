set(TARGET gtimer_init.test)

set(SOURCES
    test_gtimer_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/gtimer_init.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.status
        fwlib.lib.tmr_queue
        ms::cmocka::testrunner
        ms::lib::gtimer
        ms::lib::interrupts
        ms::lib::register_headers
        ms::lib::scp_interrupts
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    idsw_get_platform_sdv
    gtimer_prodfw_init
)
