set(TARGET i3c_target_mcp_init.test)

set(SOURCES
    test_i3c_target_mcp_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/i3c_target_mcp_init.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.driver.dw_i3c
        fwlib.lib.init
        fwlib.lib.status
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::interrupts
        ms::lib::register_headers
        ms::lib::mcp_interrupts
        ms::lib::threadx
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_compile_definitions(
    ${TARGET}
    PRIVATE
       MSCP_EXP_TOP_I3C_2_ADDRESS=MCP_EXP_TOP_I3C_2_ADDRESS  
)


lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    DfwkDeviceInitialize
    fpfw_init_get_handle
    fpfw_dw_i3c_initialize   
)