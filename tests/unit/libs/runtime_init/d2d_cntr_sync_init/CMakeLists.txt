set(TARGET d2d_cntr_sync_init.test)

set(SOURCES
    test_d2d_cntr_sync_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/d2d_cntr_sync_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        idhw
        ms::cmocka::testrunner
        ms::lib::d2d_cntr_sync
        silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    d2d_cntr_sync_init
    idsw_get_die_id
    idsw_get_platform_sdv
)
