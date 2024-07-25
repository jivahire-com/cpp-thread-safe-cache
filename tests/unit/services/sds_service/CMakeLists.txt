set(TARGET sds_service.test)

set(SOURCES
    sds_service_ut.cpp
    ${CMAKE_SOURCE_DIR}/src/services/sds_service/src/sds_api_impl.c
    ${CMAKE_SOURCE_DIR}/src/services/sds_service/src/sds_main.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.linkedlist
        fwlib.lib.utils
        fwlib.lib.status
        fwlib.lib.spinlock
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::service::sds_service
        ms::lib::error_codes
        ms::lib::crash_dump::scp
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    retrieve_sds_config_info
    DfwkInterfaceSendSync
    DfwkDeviceInitialize
    DfwkQueueInitialize
)
