set(TARGET ap_advlog_pldm.test)

set(SOURCES
    ap_advlog_pldm_tests.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(
    ${TARGET}
        PRIVATE
            ${REPO_ROOT_DIR}/src/services/ap_advlog_pldm
            ${REPO_ROOT_DIR}/src/services/cli_ap_advlog/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.service.pldm
        fwlib.service.cli
        ms::cmocka::testrunner
        ms::service::ap_advlog_pldm
        ms::service::ap_advlog_pldm::cli
        silibs::atu
        silibs::idsw
        silibs::idsw_kng
        ms::lib::utils
        ms::lib::error_codes
        ms::lib::zlib
        ms::lib::crash_dump
        ms::lib::pldm_common_headers
        ms::mocks::threadx
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    idsw_get_die_id
    idsw_get_cpu_type
    fpfw_pldm_service_raise_platform_event
    fpfw_pldm_service_register_state_effecter
    fpfw_pldm_service_state_effecter_set_complete
    atu_map
    mmio_read8
    mmio_write8
    FpFwCliRegisterTable
    _txe_event_flags_create
    _txe_event_flags_get
    _txe_event_flags_set
    _txe_thread_create
    _tx_thread_sleep
    deflateInit2_
    deflate
    deflateEnd
    zlib_alloc
    zlib_free
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)