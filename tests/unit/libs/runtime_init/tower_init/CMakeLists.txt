set(TARGET tower_init.test)

set(SOURCES
    test_tower_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/tower_init.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.init
    ms::cmocka::testrunner
    ms::lib::tower
    silibs::idsw_kng
    silibs::idhw
    silibs::kng_silibs_platform
    silibs::tower_sequence
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    idsw_get_die_id
    tower_init
)
