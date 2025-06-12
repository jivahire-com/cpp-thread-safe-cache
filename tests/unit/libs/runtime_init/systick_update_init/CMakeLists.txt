set(TARGET systick_update_init.test)

set(SOURCES
    test_systick_update_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/systick_update_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::systick_update
        silibs::platform
        silibs::idsw
        silibs::idsw_kng
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    idsw_get_platform_sdv
)
