set(TARGET fuse_init.test)

set(SOURCES
    test_fuse_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/fuse_init.c
)

add_compile_definitions(SCP_RUNTIME_INIT)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::hsp_firmware_headers
        ms::lib::scp_interrupts
        ms::lib::threadx
        ms::lib::system_info
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::atu_service
        ms::service::fuse
    PUBLIC
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.mboxPrimitives
        fwlib.driver.mboxIccTransport
        fwlib.service.cli
        fwlib.service.iccBase
        fwlib.service.icc_dispatcher
        silibs::atu
        silibs::fuse
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng  
        silibs::kng_silibs_platform
        silibs::platform

 )

target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::service::fuse,INCLUDE_DIRECTORIES>)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})


mock_target(${TARGET}
   fuse_feature_enable
   fpfw_init_get_handle
   fuse_init
   platform_fuse_override
   platform_fuse_distribution
   platform_fuse_init_cli
   system_info_get_platform_disable_cores 
   system_info_get_platform_disable_cores 
   fuse_post_mesh_init
   idhw_is_single_die_boot_en
)