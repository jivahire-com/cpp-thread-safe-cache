enabled_on(arm-eabi-aarch)

#
# Setup the SCP Runtime Init
#
set(TARGET kng.lib.scp.runtime_init.scp)

add_library(${TARGET}
    OBJECT
        accelerator_ip_init.c
        ap_cores_runtime_init.c
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
        etc_init.c
        fuse_init.c
        gtimer_init.c
        hw_version_scp_init.c
        icc_transport_init/mscp_hspmbx_init.c
        icc_cli_init.c
        mesh_init.c
        ioss_init.c
        nvic_init.c
        pcie_init.c
        textio_init.c
        vab_init.c
        ddr_manager_init.c
        power_init.c
        scp_mpu_init.c
        tower_init.c
        startup_shutdown_init.c
        usb_init.c
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
        ms::build_data
        ms::lib::scp_avs
        ms::lib::scp_avs::cli
        ms::driver::pcie
        ms::driver::sensor_fifo
        ms::driver::textio::pl011
        ms::lib::accelerator_ip
        ms::lib::ap_cores_init
        ms::lib::cortex_m7::mpu
        ms::lib::crash_dump::scp
        ms::lib::css
        ms::lib::ddrss_init
        ms::lib::debug
        ms::lib::cortex_m7::nvic
        ms::lib::event_trace::collector
        ms::lib::gtimer
        ms::lib::icc::cli
        ms::lib::mesh
        ms::lib::ioss_init
        ms::lib::ioss::usb
        ms::lib::scp_interrupts
        ms::lib::stdio_textio
        ms::lib::threadx
        ms::lib::ddr::cli
        ms::lib::vab
        ms::lib::tower
        ms::service::ddr_manager
        ms::service::fuse
        ms::service::pcie_manager
        ms::service::power
        ms::service::scp_startup_shutdown
        ms::service::sensor_fifo
        ms::service::sensor_fifo::cli
        silibs::avs
        silibs::common::lib_uart_pl011
        silibs::idhw
        silibs::idsw
        silibs::platform
        silibs::kng_silibs_platform
        ms::lib::power::cli
        ms::lib::register_headers
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       MSCP2HSP_MAILBOX_BASE_ADDRESS=SCP_TOP_SCP2HSP_MAILBOX_ADDRESS
       SCP_RUNTIME_INIT
)

# Define base addresses during compile time for respective core fw
if($ENV{REPO_APP_MSCP_VUART})
target_compile_definitions(
    ${TARGET}
    PRIVATE
       UART_BASE_ADDR=SCP_TOP_DBGR_SCP_UART_ADDRESS
)
else()
target_compile_definitions(
    ${TARGET}
    PRIVATE
        UART_BASE_ADDR=SCP_TOP_SCP_UART_ADDRESS
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
        etc_init.c
        icc_cli_init.c
        icc_transport_init/mscp_hspmbx_init.c
        mcp_mpu_init.c
        nvic_init.c
        sensor_fifo_init.c
        textio_init.c
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
        ms::build_data
        ms::driver::sensor_fifo
        ms::driver::textio::pl011
        ms::lib::debug
        ms::lib::cortex_m7::nvic
        ms::lib::crash_dump::mcp
        ms::lib::event_trace::collector
        ms::lib::icc::cli
        ms::lib::stdio_textio
        ms::lib::threadx
        ms::service::sensor_fifo
        ms::service::sensor_fifo::cli
        silibs::common::lib_uart_pl011
        silibs::idhw
        silibs::idsw
        silibs::platform
        silibs::kng_silibs_platform
        register_headers
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       MSCP2HSP_MAILBOX_BASE_ADDRESS=MCP_TOP_MCP2HSP_MAILBOX_ADDRESS
       MCP_RUNTIME_INIT
)

# Define base addresses during compile time for respective core fw
if($ENV{REPO_APP_MSCP_VUART})
target_compile_definitions(
    ${TARGET}
    PRIVATE
       UART_BASE_ADDR=MCP_TOP_DBGR_MCP_UART_ADDRESS
)
else()
target_compile_definitions(
    ${TARGET}
    PRIVATE
        UART_BASE_ADDR=MCP_TOP_MCP_UART_ADDRESS
)
endif()

lint_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::runtime_init::mcp ALIAS ${TARGET})
