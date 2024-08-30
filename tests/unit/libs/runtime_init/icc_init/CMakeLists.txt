set(TARGET icc_init.test)

set(SOURCES
    test_icc_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/icc_init.c
)

set(LIBRARIES
    fwlib.dfwk
    fwlib.lib.init
    fwlib.lib.mboxPrimitives
    fwlib.platform.dfwkhost.threadx
    fwlib.lib.platform.impl.timers.threadx
    fwlib.lib.utils
    fwlib.driver.mboxIccTransport
    fwlib.service.icc_dispatcher
    fwlib.service.iccBase
    ms::cmocka::testrunner
    ms::lib::threadx
    ms::lib::scp_interrupts
    ms::lib::icc_platform_defines
    silibs::platform
    silibs::kng_silibs_platform
    silibs::idhw
    silibs::idsw
    silibs::idsw_kng
    silibs::spi_bridge
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:fwlib.service.iccBase,INCLUDE_DIRECTORIES>)
target_link_libraries(${TARGET} PRIVATE ${LIBRARIES})

# Generic threadx lock port provided by fwlibs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_LOCK "fwlib.lib.platform.impl.lock.threadx")

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "ms::lib::threadx")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

# Provide compile time definition
target_compile_definitions(
    ${TARGET}
    PRIVATE
       MSCP2HSP_MAILBOX_BASE_ADDRESS=1000
       HW_INT_HSP2MSCP_MBOX_INT=0
       MSCP_TOP_MSCP_EXP_ADDRESS=1000
       MSCP_EXP_TOP_MSCP_MBOX_ADDRESS=1000
       MSCP_EXP_TOP_SPI_CTRL_ADDRESS=0
       MSCP_EXP_TOP_SPI_BRIDGE_ADDRESS=0
       # @todo remove after SVP updates it's HSP mbx intr number
       HW_INT_HSP2MSCP_MBOX_INT_SVP=0
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    DfwkDeviceInitialize
    fpfw_init_get_handle
    fpfw_mbox_icc_transport_dfwk_device_init
    fpfw_mbox_icc_transport_dfwk_interface_init
    fpfw_icc_base_init
    fpfw_icc_dispatcher_start
    idsw_get_platform_sdv
    idsw_get_cpu_type
    spi_bridge_init
    spi_bridge_check_errors
    spi_bridge_clear_error_interrupts
    spi_controller_init
    spi_controller_check_errors
)
