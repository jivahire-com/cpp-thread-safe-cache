set(TARGET pcie_init.test)

set(SOURCES
    test_pcie_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/pcie_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.dfwk
    fwlib.lib.init
    fwlib.platform.dfwkhost.threadx
    ms::cmocka::testrunner
    ms::driver::pcie
    ms::lib::threadx
    ms::service::pcie_manager
    silibs::idsw
    silibs::idsw_kng
    silibs::kng_silibs_platform
)

if(i386-pc-windows-msvc)
    set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
    set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
endif()

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# mock the functions needed
mock_target(${TARGET}
    fpfw_init_get_handle
    idsw_get_platform_sdv
    scp_pcie_initialize
)
