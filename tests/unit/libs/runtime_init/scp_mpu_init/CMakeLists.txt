set(TARGET scp_mpu_init.test)

set(SOURCES
    test_scp_mpu_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/scp_mpu_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.init
        fwlib.utils
        ms::cmocka::testrunner
        ms::cmsis::interface 
        ms::lib::cortex_m7::mpu
        silibs::platform
        silibs::kng_silibs_platform
        register_headers
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    ARM_MPU_Load
)
