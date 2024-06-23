set(TARGET sds_init.test)

set(SOURCES
    test_sds_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/sds_init.c
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
    fpfw_init_get_handle
    sds_init
    sds_interface_init
)
