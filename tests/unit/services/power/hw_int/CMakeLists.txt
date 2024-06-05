set(TARGET power.test.hw_int)

# Compile test
add_executable(${TARGET}
    power_hw_int_test.cpp
    power_hw_int_telemetry_test.cpp
    ../mocks/mock_dvfs.c
    ../mocks/mock_odcm.c
    ../mocks/mock_pvt.c
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::scp_avs
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::power
    PUBLIC
        fwlib.lib.assert
        fwlib.dfwk
        fwlib.lib.linkedlist
        fwlib.lib.utils
 )

# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::service::power,INCLUDE_DIRECTORIES>
        ../
        ../mocks/

)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    dvfs_get_adclk_droop_count
    dvfs_init
    dvfs_pll_dsm_toggle_start
    dvfs_pll_dsm_toggle_end
    dvfs_set_plimit
    dvfs_telemetry_config
    odcm_init
    odcm_telemetry_config
    reset_tile_pvt_dts_vm
    soc_pvt_init
    tile_pvt_dma_config
    tile_pvt_init
    tile_pvt_sda_reconfig
    wait_for_FLLCalDone
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)