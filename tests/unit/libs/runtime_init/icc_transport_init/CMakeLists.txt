set(TARGET icc_transport_mscp_hspmbx_init.test)

set(SOURCES
    test_mscp_hspmbx_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/icc_transport_init/mscp_hspmbx_init.c
)

set(LIBRARIES
    fwlib.dfwk
    fwlib.lib.init
    fwlib.lib.mboxPrimitives
    fwlib.platform.dfwkhost.threadx
    fwlib.lib.platform.impl.timers.threadx
    fwlib.lib.utils
    fwlib.driver.mboxIccTransport
    ms::cmocka::testrunner
    ms::lib::threadx
    silibs::platform
    silibs::kng_silibs_platform
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET} PRIVATE ${LIBRARIES})

# Generic threadx lock port provided by fwlibs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_LOCK "fwlib.lib.platform.impl.lock.threadx")

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "ms::lib::threadx")

# Provide compile time definition
target_compile_definitions(${TARGET} PRIVATE MSCP2HSP_MAILBOX_BASE_ADDRESS=1000)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    DfwkDeviceInitialize
    fpfw_init_get_handle
    fpfw_mbox_icc_transport_dfwk_device_init
    fpfw_mbox_icc_transport_dfwk_interface_init
)
