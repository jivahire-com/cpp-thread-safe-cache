set(TARGET ioss_init.test)

set(SOURCES
    test_ioss_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/ioss_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        clocks_sequence
        fwlib.lib.init
        idhw
        fwlib.lib.utils
        idsw
        ms::cmocka::testrunner
        ms::lib::ioss_init
        silibs_platform
        kng_silibs_platform
        fwlib.lib.assert
        ppu_v1
        atu
        pcr_ioss
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    ioss_init
    idhw_get_die_id
    idsw_get_platform_sdv
)
