set(TARGET vab_init.test)

set(SOURCES
    test_vab_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/vab_init.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.init
    ms::cmocka::testrunner
    ms::lib::vab
    silibs::idsw
    silibs::kng_silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    idsw_get_platform_sdv
    vab_init
)
