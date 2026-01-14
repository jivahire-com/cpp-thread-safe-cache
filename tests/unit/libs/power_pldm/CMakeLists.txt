# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET power_pldm.test)

set(SOURCES
    test_power_pldm.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})
target_compile_options(${TARGET} PRIVATE -Wno-c99-designator -Wno-initializer-overrides)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.cfg_mgr
        fwlib.service.iccBase
        fwlib.service.pldm
        ms::cmocka::testrunner
        ms::lib::crash_dump
        ms::lib::hsp_firmware_headers
        ms::lib::pldm_common_headers
        ms::lib::pldm_pdr
        ms::lib::register_headers
        ms::lib::system_info
        ms::service::fuse
        ms::service::pwr_pldm_mcp
        silibs::fuse
        silibs::icc_mhu
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
    fpfw_icc_base_send
    fpfw_icc_base_recv
    FpFwLockAcquire
    FpFwLockRelease
)