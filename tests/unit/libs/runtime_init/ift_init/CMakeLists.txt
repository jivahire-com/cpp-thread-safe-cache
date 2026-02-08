# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET ift_init.test)

set(SOURCES
    test_ift_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/ift_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.interface.platform.threadx
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.init
        fwlib.lib.utils
        ms::lib::ift
        ms::lib::scp_interrupts
        ms::cmocka::testrunner
        silibs::idsw_kng
        silibs::idsw
        silibs::platform
        silibs::sdm_command_desc
)

# Generic ports implmented within this project
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
    fpfw_init_get_handle
    ift_dfwk_device_initialize
    ift_dfwk_interface_initialize
    ift_dfwk_set_interface
    ift_init
    ift_is_enabled
)
