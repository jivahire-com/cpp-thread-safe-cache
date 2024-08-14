set(TARGET tower_init.test)

set(SOURCES
    test_tower_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/tower_init.c
)

add_executable(${TARGET} ${SOURCES})
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

# @todo remove after completing task https://azurecsi.visualstudio.com/Dev/_workitems/edit/1933312/
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:fwlib.service.iccBase,INCLUDE_DIRECTORIES>)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.assert
    fwlib.lib.init
    fwlib.service.iccBase
    fwlib.driver.mboxIccTransport
    fwlib.lib.mboxPrimitives
    ms::cmocka::testrunner
    ms::lib::tower
    ms::lib::hsp_firmware_headers
    ms::lib::system_info
    silibs::idsw_kng
    silibs::idhw
    silibs::kng_silibs_platform
    silibs::tower_sequence
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    fpfw_init_get_handle
    idsw_get_die_id
    tower_init
)
