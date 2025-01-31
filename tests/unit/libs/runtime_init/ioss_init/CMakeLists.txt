set(TARGET ioss_init.test)

set(SOURCES
    test_ioss_ini.cpp
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
        silibs::idsw
        silibs::idsw_kng
        ms::cmocka::testrunner
        ms::lib::ioss_ini
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
    ioss_ini
    idsw_get_die_id
)
