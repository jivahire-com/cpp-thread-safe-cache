set(TARGET kng_mscp_boot_loader.test)

set(SOURCES
    test_kingsgate_boot.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/boot_loader/kingsgate_boot.c
    ${CMAKE_SOURCE_DIR}/src/libs/boot_loader/unpack_image.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.mboxPrimitives
    ms::cmocka::testrunner
    ms::boot_loader::interface
    ms::lib::hsp_firmware_headers
    ms::lib::system_info
    ms::lib::zlib
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    FpFwMailboxInit
    FpFwMailboxFlushFIFO
    FpFwMailboxReceive
    FpFwMailboxSend
    system_info_is_hsp_present
    __disable_irq
)
