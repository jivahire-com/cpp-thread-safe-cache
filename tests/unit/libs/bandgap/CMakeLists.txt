set(TARGET bandgap.test)

set(SOURCES
    test_bandgap.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::bandgap
        ms::lib::scp_interrupts
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    mmio_read32
    mmio_write32
)
