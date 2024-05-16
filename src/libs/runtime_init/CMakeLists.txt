enabled_on(arm-eabi-aarch)

#
# Setup the SCP Runtime Init
#
set(TARGET kng.lib.scp.runtime_init.scp)

add_library(${TARGET} 
    OBJECT 
        accelerator_ip_init.c
        avs_scp_init.c
        cli_init.c
        cli_ddr_init.c
        css_init.c
        debug_init.c
        dfwk_init.c
        etc_scp_init.c
        hw_version_scp_init.c
        icc_transport_init/mscp_hspmbx_init.c
        mesh_init.c
        nvic_init.c
        pcie_init.c
        textio_init.c
        ddr_manager_init.c
        power_init.c
        scp_mpu_init.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        fwlib.cli
        fwlib.dfwk
        fwlib.drivers.interface.text_io
        fwlib.init
        fwlib.mboxprimitives
        fwlib.platform.dfwkhost.threadx
        fwlib.platform.timers.threadx
        fwlib.utils
        fwlib.Driver.MboxIccTransport
        ms::lib::scp_avs
        ms::driver::textio::pl011
        ms::driver::pcie
        ms::lib::accelerator_ip
        ms::lib::cortex_m7::mpu
        ms::lib::css
        ms::lib::debug
        ms::lib::cortex_m7::nvic
        ms::lib::icc::cli
        ms::lib::mesh
        ms::lib::scp_interrupts
        ms::lib::stdio_textio
        ms::lib::threadx
        ms::lib::ddr::cli
        ms::service::ddr_manager
        ms::service::pcie_manager
        ms::service::power
        ms::service::scp_etc
        silibs::avs
        silibs::common::lib_uart_pl011
        silibs::idhw
        silibs::idsw
        silibs::platform
        silibs::kng_silibs_platform
        register_headers
)

# Define base addresses during compile time for respective core fw
target_compile_definitions(
    ${TARGET} 
    PRIVATE 
        MSCP2HSP_MAILBOX_BASE_ADDRESS=SCP_TOP_SCP2HSP_MAILBOX_ADDRESS 
        UART_BASE_ADDR=SCP_TOP_SCP_UART_ADDRESS
)

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
        debug_init.c
        dfwk_init.c
        etc_mcp_init.c
        icc_cli_init.c
        icc_transport_init/mscp_hspmbx_init.c
        mcp_mpu_init.c
        nvic_init.c
        textio_init.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        fwlib.cli
        fwlib.dfwk
        fwlib.init
        fwlib.mboxprimitives
        fwlib.platform.dfwkhost.threadx
        fwlib.platform.timers.threadx
        fwlib.utils
        fwlib.Driver.MboxIccTransport
        ms::driver::textio::pl011
        ms::lib::debug
        ms::lib::cortex_m7::nvic
        ms::lib::icc::cli
        ms::lib::stdio_textio
        ms::lib::threadx
        ms::service::mcp_etc
        silibs::common::lib_uart_pl011
        silibs::idhw
        silibs::idsw
        silibs::platform
        silibs::kng_silibs_platform
        register_headers
)

# Define base addresses during compile time for respective core fw
target_compile_definitions(
    ${TARGET} 
    PRIVATE 
        MSCP2HSP_MAILBOX_BASE_ADDRESS=MCP_TOP_MCP2HSP_MAILBOX_ADDRESS 
        UART_BASE_ADDR=MCP_TOP_MCP_UART_ADDRESS
)

lint_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::runtime_init::mcp ALIAS ${TARGET})
