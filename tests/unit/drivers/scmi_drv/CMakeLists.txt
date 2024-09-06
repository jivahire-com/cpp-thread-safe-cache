set(TARGET kng.driver.scmi_drv.test)

set(SOURCES
    test_scmi_drv.cpp
    scmi_drv_mock.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    kng.driver.scmi_drv
    kng.driver.icc_mhu_trans	

    fwlib.dfwk
    fwlib.lib.assert
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::lib::arm_intrinsic
    ms::lib::register_headers
    ms::service::ap_core
    silibs::icc_mhu	
    silibs::kng_silibs_platform
    silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.driver.scmi_drv,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    DfwkAsyncRequestInitialize
    DfwkClientInterfaceOpen
    ap_core_core_power_off
    ap_core_core_power_on
    ap_core_set_rvbaraddr
    icc_mhu_trans_get_msg_from_index
    icc_mhu_trans_send_message_idx
    scmi_send_resp
    scmi_check_message
    scmi_power_protocol_cmds
    scmi_sys_pwr_protocol_cmds
    scmi_ap_core_protocol_cmds
)
