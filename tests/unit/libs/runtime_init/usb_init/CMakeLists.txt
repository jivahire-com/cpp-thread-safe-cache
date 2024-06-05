set(TARGET usb_init.test)

set(SOURCES
    test_usb_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/usb_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        idhw
        fwlib.lib.utils
        idsw
        ms::cmocka::testrunner
        ms::lib::ioss::usb
        silibs_platform
        kng_silibs_platform
        fwlib.lib.assert
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    idhw_get_die_id
    usb_init
    idsw_get_platform_sdv
)
