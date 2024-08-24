enabled_on(arm-eabi-aarch)

#
# Setup the SCP Runtime Init
#
set(TARGET kng.lib.scp.runtime_init.scp)

add_library(${TARGET}
    OBJECT
        accelerator_ip_init.c
        accel_intr_client_init.c
        accel_intr_service_init.c
        ap_core_init.c
        atu_init.c
        avs_scp_init.c
        avs_scp_cli_init.c
        cli_init.c
        cli_ddr_init.c
        cli_power_init.c
        crash_dump_init.c
        crash_dump_init_scp.c
        css_init.c
        ddrss_init.c
        debug_init.c
        dfwk_init.c
        etc_etd_init.c
        fuse_init.c
        gtimer_init.c
        hw_version_scp_init.c
        icc_init.c
        icc_cli_init.c
        icc_transport_init/scp_icc_mhu_trans_init.c
        mesh_init.c
        mesh_scp_cli_init.c
        ioss_init.c
        nvic_init.c
        pcie_init.c
        textio_init.c
        vab_init.c
        ddr_manager_init.c
        power_init.c
        scmi_init.c
        scp_mpu_init.c
        sensor_fifo_init.c
        tower_init.c
        startup_shutdown_init.c
        scmi_cli_init.c
        usb_init.c
        sds_init.c
        warm_start_init.c
        warm_start_cli_init.c
        gpio_init.c
        i3c_controller_init.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        fwlib.service.cli
        fwlib.lib.crashDump
        fwlib.dfwk
        fwlib.driver.interface.text_io
        fwlib.lib.init
        fwlib.lib.mboxPrimitives
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.platform.impl.timers.threadx
        fwlib.lib.utils
        fwlib.driver.mboxIccTransport
        fwlib.service.icc_dispatcher
        fwlib.service.iccBase
        ms::build_data
        ms::lib::scp_avs
        ms::lib::scp_avs::cli
        ms::driver::gpio
        ms::driver::icc_mhu_trans
        ms::driver::pcie
        ms::driver::sensor_fifo
        ms::driver::scmi_drv
        ms::driver::vab
        ms::driver::textio::pl011
        ms::lib::accelerator_ip
        ms::lib::cortex_m7::mpu
        ms::lib::crash_dump::scp
        ms::lib::css
        ms::lib::ddrss_init
        ms::lib::debug
        ms::lib::cortex_m7::nvic
        ms::lib::gtimer
        ms::lib::hsp_firmware_headers
        ms::lib::i3c_controller
        ms::lib::icc::cli
        ms::lib::icc_platform_defines
        ms::lib::mesh
        ms::lib::mesh::cli
        ms::lib::ioss_init
        ms::lib::ioss::usb
        ms::lib::scmi::cli
        ms::lib::scp_interrupts
        ms::lib::stdio_textio
        ms::lib::system_info
        ms::lib::threadx
        ms::lib::ddr::cli
        ms::lib::tower
        ms::service::accel_intr_client
        ms::service::accel_intr_service
        ms::service::ap_core
        ms::service::ddr_manager
        ms::service::event_trace::collector
        ms::service::event_trace::decoder
        ms::service::fuse
        ms::service::pcie_manager
        ms::service::pcie_manager::cli
        ms::service::power
        ms::service::scp_startup_shutdown
        ms::service::sensor_fifo
        ms::service::sensor_fifo::cli
        silibs::avs
        silibs::common::lib_uart_pl011
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::platform
        silibs::pcie
        silibs::kng_silibs_platform
        silibs::sensor_ram_bridge
        ms::lib::power::cli
        ms::lib::register_headers
        ms::service::sds_service
        ms::lib::warm_start
        ms::lib::warm_start::cli
        ms::service::atu_service
)

target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:fwlib.service.iccBase,INCLUDE_DIRECTORIES>)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       MSCP2HSP_MAILBOX_BASE_ADDRESS=SCP_TOP_SCP2HSP_MAILBOX_ADDRESS
       HW_INT_HSP2MSCP_MBOX_INT=HW_INT_HSP2SCP_MB_SW_INT
       SCP_RUNTIME_INIT
)

# Define base addresses during compile time for respective core fw
if($ENV{REPO_APP_MSCP_VUART})
target_compile_definitions(
    ${TARGET}
    PRIVATE
       UART_BASE_ADDR=SCP_TOP_DBGR_SCP_UART_ADDRESS
       UART_IRQ=HW_INT_SCP_VUART_INT
)
else()
target_compile_definitions(
    ${TARGET}
    PRIVATE
        UART_BASE_ADDR=SCP_TOP_SCP_UART_ADDRESS
        UART_IRQ=HW_INT_SCP_UART_INT
)
endif()

lint_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::runtime_init::scp ALIAS ${TARGET})

#
# Setup the MCP Runtime Init
#
set(TARGET kng.lib.scp.runtime_init.mcp)

add_library(${TARGET}
    OBJECT
        cli_init.c
        crash_dump_init.c
        debug_init.c
        dfwk_init.c
        etc_etd_init.c
        hw_version_mcp_init.c
        icc_cli_init.c
        icc_init.c
        mcp_mpu_init.c
        nvic_init.c
        textio_init.c
        gpio_init.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        fwlib.service.cli
        fwlib.lib.crashDump
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.mboxPrimitives
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.platform.impl.timers.threadx
        fwlib.lib.utils
        fwlib.driver.mboxIccTransport
        fwlib.service.icc_dispatcher
        fwlib.service.iccBase
        ms::build_data
        ms::driver::gpio
        ms::driver::sensor_fifo
        ms::driver::textio::pl011
        ms::lib::debug
        ms::lib::cortex_m7::nvic
        ms::lib::crash_dump::mcp
        ms::lib::icc::cli
        ms::lib::icc_platform_defines
        ms::lib::mcp_interrupts
        ms::lib::stdio_textio
        ms::lib::threadx
        ms::service::event_trace::collector
        ms::service::event_trace::decoder
        ms::service::sensor_fifo
        ms::service::sensor_fifo::cli
        ms::lib::cortex_m7::mpu
        silibs::common::lib_uart_pl011
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::platform
        silibs::kng_silibs_platform
        silibs::sensor_ram_bridge
        register_headers
)

target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:fwlib.service.iccBase,INCLUDE_DIRECTORIES>)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       MSCP2HSP_MAILBOX_BASE_ADDRESS=MCP_TOP_MCP2HSP_MAILBOX_ADDRESS
       HW_INT_HSP2MSCP_MBOX_INT=HW_INT_HSP2MCP_MB_SW_INT
       MCP_RUNTIME_INIT
)

# Define base addresses during compile time for respective core fw
if($ENV{REPO_APP_MSCP_VUART})
target_compile_definitions(
    ${TARGET}
    PRIVATE
       UART_BASE_ADDR=MCP_TOP_DBGR_MCP_UART_ADDRESS
       UART_IRQ=HW_INT_MCP_VUART_INT
)
else()
target_compile_definitions(
    ${TARGET}
    PRIVATE
        UART_BASE_ADDR=MCP_TOP_MCP_UART_ADDRESS
        UART_IRQ=HW_INT_MCP_UART_INT
)
endif()

lint_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::runtime_init::mcp ALIAS ${TARGET})
