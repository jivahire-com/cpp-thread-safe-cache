set(TARGET ddrss_init.test)

set(SOURCES
    test_ddrss_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/ddrss_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.init
        idhw
        ms::cmocka::testrunner
        ms::lib::ddrss_init
        silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    ddrss_lib_init
    idhw_get_die_id
)
