set(TARGET rhtlm_service.test)

# Compile test
add_executable(${TARGET}
    rhtlm_service_test.cpp
    ${CMAKE_SOURCE_DIR}/src/services/rhtlm_service/rhtlm_service.c
)

target_link_libraries(${TARGET}
    PRIVATE
    ms::cmocka::testrunner
    ms::mocks::event_trace
)

# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
    $<TARGET_PROPERTY:kng.service.rhtlm_service,INCLUDE_DIRECTORIES>
)

# Defining platform APIs
lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# mock the functions needed
mock_target(${TARGET}
    _txe_thread_create
    _tx_thread_sleep
    config_get_rh_tlm_cca_mode
    config_get_rm_tm_rec_mask
    config_get_rh_tlm_service_period_ms
    config_get_erhm_en
    config_get_intu_init_en
    idsw_get_die_id
    ddrss_get_telemetry_record
    prod_ddrss_convert_rh_rec_to_rh_cper
    ddrss_set_rhm_telemetry_record_mask
    hm_submit_cper
    FpFwCliPrint
    FPFwErrorRaise
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)
