set(TARGET ddr_manager.test)

set(SOURCES
    ddr_manager_bwl_test.cpp
    ddr_manager_i3c_test.cpp
    ddr_manager_polling_test.cpp
    ddr_manager_test.cpp
    ddr_memory_map_test.cpp
    ddr_mocks.cpp
)

set(LIBS
    PRIVATE
        idhw
        ms::cmocka::testrunner
        ms::lib::ddr_manager
        ms::lib::error_handler
        ms::lib::hsp_firmware_headers
        ms::lib::threadx
        ms::mocks::threadx
        ms::lib::stdio_textio
        ms::driver::textio::pl011
        silibs::ddr_i3c
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.lib.spinlock
        fwlib.service.iccBase
        ras_arm
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
        ${LIBS}
)

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::lib::ddr_manager,INCLUDE_DIRECTORIES>)

# Defining platform APIs
# set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
# set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.interface.win32")
# set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})
mock_target(${TARGET}
    _txe_mutex_create
    _txe_queue_create
    _txe_thread_create
    _txe_queue_send
    _txe_queue_receive
    _txe_timer_create
    ddr_i3c_interface_read_temp_sensor_mr_reg
    ddr_i3c_interface_get_instance
    ddr_manager_platform_is_polling_supported
    fpfw_icc_base_send_recv_sync
    idhw_get_die_id
    prod_ddrss_lib_init
    system_info_is_hsp_present
    mmio_read32
    mmio_write32
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)
