# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET spi_bridge_init.test)

set(SOURCES
    test_spi_bridge_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/spi_bridge_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET} 
    PRIVATE 
    fwlib.lib.init
    fwlib.lib.status
    fwlib.lib.utils
    ms::cmocka::testrunner
    ms::lib::boot_status
    silibs::idsw_kng
    silibs_platform
    silibs::kng_silibs_platform
    ms::lib::spi
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
       MSCP_TOP_MSCP_EXP_ADDRESS=1000
       MSCP_EXP_TOP_SPI_CTRL_ADDRESS=0
       MSCP_EXP_TOP_SPI_BRIDGE_ADDRESS=0
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    spi_bridge_init
    spi_bridge_check_errors
    spi_bridge_clear_error_interrupts
    spi_controller_init
    spi_controller_check_errors
    boot_status_notify_extd
    idsw_get_cpu_type
    idsw_get_die_id
)
