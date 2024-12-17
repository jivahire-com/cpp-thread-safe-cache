set(TARGET ddrss_init.test)

set(SOURCES
    test_ddrss_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/ddrss_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

if (i386-pc-windows-msvc)
    set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
    set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
endif()

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        idhw
        ms::cmocka::testrunner
        ms::lib::ddr_manager
        ms::lib::ddrss_init
        silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    FPFwCoreInterruptEnableVector
    FPFwCoreInterruptRegisterCallback
    ddr_manager_init
    idhw_get_die_id
    idsw_get_die_id
    idhw_is_single_die_boot_en
)
