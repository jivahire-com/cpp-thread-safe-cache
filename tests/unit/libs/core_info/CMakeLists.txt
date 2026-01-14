# Copyright (c) Microsoft Corporation. All rights reserved.

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
        ms::service::fuse
        silibs::fuse
        silibs::kng_silibs_platform
        silibs::platform
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
   idsw_get_die_id
   read_core_defect_fuses
   config_get_die0_core_disable_value_31_0
   config_get_die0_core_disable_value_63_32
   config_get_die0_core_disable_value_95_64
   config_get_die1_core_disable_value_31_0
   config_get_die1_core_disable_value_63_32
   config_get_die1_core_disable_value_95_64
   config_get_die0_core_spare_en_31_0
   config_get_die0_core_spare_en_63_32
   config_get_die0_core_spare_en_95_64
   config_get_die1_core_spare_en_31_0
   config_get_die1_core_spare_en_63_32
   config_get_die1_core_spare_en_95_64
) 