set(TARGET icc_init.test)

set(SOURCES
    test_icc_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/icc_hspmbx_init.c
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/icc_d2dmbx_init.c
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/icc_accel_mbx_init.c
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/icc_mhu_init.c
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
    ms::driver::icc_mhu_trans
    ms::lib::threadx
    ms::lib::scp_interrupts
    ms::lib::icc_platform_defines
    ms::lib::interface::mhu_icc_payloads
    ms::lib::accel_intr
    ms::service::atu_service
    silibs::platform
    silibs::kng_silibs_platform
    silibs::icc_mhu
    silibs::idhw
    silibs::idsw
    silibs::idsw_kng
    silibs::sdm_command_desc
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
       HW_INT_CDED_MBOX_INT=118
       HW_INT_SDM_MBOX_INT=119
       HW_INT_MSCP2MSCP_MHU_INT=120
       HW_INT_AP2MSCP_MHU_S_INT=121
       HW_INT_MSCP2AP_NS_MHU_INT=122
       MSCP_TOP_MSCP_EXP_ADDRESS=1000
       MSCP_EXP_TOP_MSCP_MBOX_ADDRESS=1000
       MSCP_EXP_TOP_SPI_CTRL_ADDRESS=0
       MSCP_EXP_TOP_SPI_BRIDGE_ADDRESS=0
       RMSS_D2D_SCP_MB_HW_INT=0
       ACCEL_MBOX_OFFSET_AFTER_0X100000=1000
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    DfwkDeviceInitialize
    accel_intr_init
    fpfw_init_get_handle
    fpfw_mbox_icc_transport_dfwk_device_init
    fpfw_mbox_icc_transport_dfwk_interface_init
    fpfw_icc_base_init
    fpfw_icc_dispatcher_start
    idhw_get_die_id
    idsw_get_platform_sdv
    idsw_get_die_id
    idsw_get_cpu_type
    mhu_icc_transport_device_init
    mhu_icc_transport_interface_init
    spi_bridge_init
    spi_bridge_check_errors
    spi_bridge_clear_error_interrupts
    spi_controller_init
    spi_controller_check_errors
)
