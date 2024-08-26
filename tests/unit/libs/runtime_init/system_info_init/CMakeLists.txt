set(TARGET system_info_init.test)

set(SOURCES
    test_system_info_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/system_info_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/libs/system_info/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        ms::lib::system_info
        ms::cmocka::testrunner
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    system_info_init
)
