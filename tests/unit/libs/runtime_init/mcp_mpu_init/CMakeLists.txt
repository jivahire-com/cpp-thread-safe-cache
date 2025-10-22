set(TARGET mcp_mpu_init.test)

set(SOURCES
    test_mcp_mpu_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/mcp_mpu_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::cmsis::interface
        ms::lib::cortex_m7::mpu
        ms::service::atu_service
        silibs::platform
        silibs::kng_silibs_platform
        register_headers
)

# Generic ports implemented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    ARM_MPU_Load
)
