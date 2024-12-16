set(TARGET power_fuse.test)

add_executable(${TARGET}
   test_power_fuse.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::scp_interrupts
        ms::lib::system_info
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::lib::power_fuse
    
    PUBLIC
        fwlib.lib.assert
        silibs::fuse
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::platform

 )

target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::lib::power_fuse,INCLUDE_DIRECTORIES>)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.lib.power_fuse,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    idsw_get_platform_sdv
    read_fuse
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)
