set(TARGET kng_mscp_unpack_image.test)

set(SOURCES
    test_unpack_image.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/boot_loader/unpack_image.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

target_compile_definitions(${TARGET} PUBLIC UNIT_TEST)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
    ms::cmocka::testrunner
    ms::lib::boot_loader
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})