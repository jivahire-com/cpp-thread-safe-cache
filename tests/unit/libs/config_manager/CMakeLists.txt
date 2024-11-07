set(TARGET config_manager.test)

set(SOURCES
    config_manager_ut.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_include_directories(${TARGET} 
    PRIVATE 
        $<TARGET_PROPERTY:ms::lib::config_manager,INCLUDE_DIRECTORIES>
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.lib.status
        fwlib.lib.spinlock
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::lib::error_codes
        ms::lib::crash_dump
        ms::lib::config_manager
        ms::lib::config_manager::cli
        ms::lib::variable_services
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    system_info_is_hsp_present
    variable_service_initialize_ctx
    variable_service_sync_get_variable
    variable_service_sync_set_variable
    variable_service_unlock_get_var_ctx
    update_knob_in_cached_db_cb
)
