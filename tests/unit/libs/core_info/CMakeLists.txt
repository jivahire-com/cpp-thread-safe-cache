set(TARGET core_info.test)

set(SOURCES
    test_core_info.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})
target_compile_options(${TARGET} PRIVATE -Wno-c99-designator -Wno-initializer-overrides)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.service.iccBase
        fwlib.lib.cfg_mgr
        ms::cmocka::testrunner
        ms::lib::hsp_firmware_headers
        ms::lib::core_info
        ms::lib::register_headers
        ms::lib::system_info
        silibs::fuse
        silibs::kng_silibs_platform
        silibs::platform
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
   read_core_defect_fuses
   config_get_core_disable_value_0_31
   config_get_core_disable_value_32_63
   config_get_core_disable_value_64_95
   config_get_core_spare_en_0_31
   config_get_core_spare_en_32_63
   config_get_core_spare_en_64_95
) 