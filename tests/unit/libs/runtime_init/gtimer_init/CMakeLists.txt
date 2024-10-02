set(TARGET gtimer_init.test)

set(SOURCES
    test_gtimer_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/gtimer_init.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        ms::cmocka::testrunner
        ms::lib::gtimer
        ms::lib::register_headers
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    idsw_get_platform_sdv
    gtimer_init
)
