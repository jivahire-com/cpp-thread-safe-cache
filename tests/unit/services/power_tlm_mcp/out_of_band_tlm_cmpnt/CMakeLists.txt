set(TARGET  kng.service.pwr_tlm_mcp.cmpnt.out_of_band_tlm.test)

set(SOURCES
    out_of_band_mocks.c
    test_out_of_band_cmpnt.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    kng.service.pwr_tlm_mcp.cmpnt.out_of_band_tlm
    fwlib.lib.status
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::mocks::threadx

)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.pwr_tlm_mcp,INCLUDE_DIRECTORIES>)
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.pwr_tlm_mcp.cmpnt.out_of_band_tlm,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    fpfw_pldm_service_register_numeric_sensor
    fpfw_pldm_service_numeric_sensor_set
    fpfw_pldm_service_numeric_sensor_not_ready
    FpFwAssertWithArgs
    _txe_event_flags_create
    _txe_event_flags_get
    _txe_event_flags_set
    exec_tlm_cmpnt_is_oob_data_valid
    exec_tlm_cmpnt_notify_new_out_of_band_pldm_request
    data_proc_tlm_cmpnt_get_oob_crit_max_soc_temp_dC
    data_proc_tlm_cmpnt_get_oob_soc_pwr_mW
    data_proc_tlm_cmpnt_get_oob_crit_max_dimm_temp_dC
    data_proc_tlm_cmpnt_get_oob_dimm_total_pwr_mW
    data_proc_tlm_cmpnt_get_oob_crit_max_vr_temp_dC
)