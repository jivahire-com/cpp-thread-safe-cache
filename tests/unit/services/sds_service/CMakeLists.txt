set(TARGET sds_service.test)

set(SOURCES
    sds_service_ut.cpp
    ${CMAKE_SOURCE_DIR}/src/services/sds_service/src/sds_api_impl.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.linkedlist
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::service::sds_service
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    DfwkInterfaceSendSync
)
