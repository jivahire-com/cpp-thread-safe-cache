set(TARGET etc_mcp_init.test)

set(SOURCES
    test_etc_mcp_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/etc_mcp_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.init
        idhw
        ms::cmocka::testrunner
        ms::lib::event_trace::trace
        ms::mocks::event_trace
        ms::service::mcp_etc
        silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    mcp_etc_initialize
)
