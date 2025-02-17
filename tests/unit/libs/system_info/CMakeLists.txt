set(TARGET system_info.test)

set(SOURCES
    test_system_info.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})
target_compile_options(${TARGET} PRIVATE -Wno-c99-designator -Wno-initializer-overrides)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.service.iccBase
        ms::cmocka::testrunner
        ms::lib::hsp_firmware_headers
        ms::lib::register_headers
        ms::lib::system_info
        silibs::kng_silibs_platform
        silibs::platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
   fpfw_icc_base_send_recv_sync
   mmio_read32
) 