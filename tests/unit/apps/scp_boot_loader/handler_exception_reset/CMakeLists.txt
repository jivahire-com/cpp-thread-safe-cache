set(TARGET handler_scp_exception_reset.test)

set(SOURCES
    test_handler_exception_reset.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/scp_boot_loader/src/handlers.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

target_compile_definitions(${TARGET} PUBLIC UNIT_TEST)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
    ms::cmocka::testrunner
	boot_loader_interface
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    _start
)
