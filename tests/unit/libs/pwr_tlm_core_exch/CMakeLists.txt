# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.lib.pwr_tlm_core_ex.test)

add_executable(${TARGET}
   test_pwr_tlm_exch.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.utils
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::pwr_tlm_core_ex

    PUBLIC
        silibs::fuse
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::platform
 )

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}

)

