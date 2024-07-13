set(TARGET kng.driver.icc_mhu_trans.test)

set(SOURCES
    test_icc_mhu_trans.cpp
    icc_mhu_mock.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    kng.driver.icc_mhu_trans

    fwlib.dfwk
    fwlib.lib.assert
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::lib::arm_intrinsic
    ms::lib::register_headers	
    silibs::icc_mhu
    silibs::mhuv3
    silibs::kng_silibs_platform
    silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.driver.icc_mhu_trans,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    icc_mhu_send_message
    icc_mhu_get_message
	mhuv3_clr_mdbcw_status
	icc_mhu_trans_get_message
)
