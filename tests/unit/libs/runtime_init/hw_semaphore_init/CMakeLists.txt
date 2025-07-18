set(TARGET hw_semaphore_init.test)

set(SOURCES
    test_hw_semaphore_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/hw_semaphore_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::crash_dump
        ms::lib::hw_semaphore
        silibs_platform
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    idhw_is_single_die_boot_en
    set_semaphore_group_base
    idsw_get_die_id
    idsw_get_cpu_type
    initialize_semaphore
)
