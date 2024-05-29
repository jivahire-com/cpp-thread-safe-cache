set(TARGET usb.test)

set(SOURCES
    test_usb.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.init
        ms::cmocka::testrunner
        ms::cmocka::mocks
        ms::lib::ioss::usb
        ms::lib::error_handler
        silibs_platform
        kng_silibs_platform
        fwlib.assert
        idsw
        usb_init
        silibs::ioss
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    usbss_init
)