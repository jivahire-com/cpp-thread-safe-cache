set(TARGET avs_scp_init.test)

set(SOURCES
    test_avs_scp_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/avs_scp_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.datastructures.linkedlist
        fwlib.utils
        ms::cmocka::testrunner
        ms::lib::threadx
        scp_avs
        silibs::avs
        silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    scp_avs_init
    scp_avs_interface_initialize
    fpfw_init_get_handle
)
