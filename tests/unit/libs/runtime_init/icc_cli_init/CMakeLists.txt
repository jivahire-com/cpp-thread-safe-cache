set(TARGET icc_cli_init.test)

set(SOURCES
    test_icc_cli_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/icc_cli_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.service.cli
        fwlib.lib.init
        fwlib.lib.utils
        ms::lib::icc::cli
        ms::cmocka::testrunner
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::platform
        silibs::kng_silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    icc_cli_init
    idsw_get_die_id
    idsw_get_platform_sdv
    idsw_get_cpu_type
    idhw_is_single_die_boot_en
)
