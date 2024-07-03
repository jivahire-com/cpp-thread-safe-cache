set(TARGET fuse.test)

add_executable(${TARGET}
   test_fuse.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::scp_interrupts
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::fuse
    PUBLIC
        fwlib.lib.assert
        silibs::fuse
        silibs::idsw
        silibs::idsw_kng  
        silibs::kng_silibs_platform
        silibs::platform

 )


# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::service::fuse,INCLUDE_DIRECTORIES>)
#target_compile_definitions(${TARGET} PUBLIC UNIT_TEST)
# Defining platform APIs
#set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
#set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    idsw_get_platform_sdv
    fuse_dma_copy_to_ram_blocking
    read_fuse
    apply_fuse_override
    distribute_fuses
    trigger_debugger_for_manual_overrides
    #apply_fuse_override_ignoring_valids
    fuse_dist_get_exclusion_list
    fuse_get_exclude_list_fpga
)


