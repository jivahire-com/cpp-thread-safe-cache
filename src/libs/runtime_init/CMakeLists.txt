enabled_on(arm-eabi-aarch)

# 
# Setup the SCP Runtime Init
# 
set(TARGET kng.lib.scp.runtime_init.scp)

add_library(${TARGET} 
    OBJECT 
        css_init.c
        debug_init.c
        dfwk_init.c
        etc_scp_init.c
        hw_version_scp_init.c
        icc_transport_init/mscp_hspmbx_init.c
        mesh_init.c
        nvic_init.c
        textio_init.c
)

target_link_libraries(
    ${TARGET} 
    PRIVATE
        fwlib.dfwk
        fwlib.init
        fwlib.mboxprimitives
        fwlib.platform.dfwkhost.threadx
        fwlib.platform.timers.threadx
        fwlib.utils
        fwlib.Driver.MboxIccTransport
        ms::driver::textio::pl011
        ms::lib::css
        ms::lib::debug
        ms::lib::cortex::m7::nvic
        ms::lib::mesh
        ms::lib::stdio_textio
        ms::lib::threadx
        ms::service::scp_etc
        silibs::idhw
        silibs::idsw
        silibs::platform
        silibs::kng_silibs_platform        
)

# Ensure ms::lib::arm_utils precedes ICC transport driver in linker invocation by specifying this link dependency edge
target_link_libraries(fwlib.Driver.MboxIccTransport PRIVATE ms::lib::arm_utils)
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
        debug_init.c
        dfwk_init.c
        etc_mcp_init.c
        icc_transport_init/mscp_hspmbx_init.c
        nvic_init.c
        textio_init.c
)

target_link_libraries(
    ${TARGET} 
    PRIVATE
        fwlib.dfwk
        fwlib.init
        fwlib.mboxprimitives
        fwlib.platform.dfwkhost.threadx
        fwlib.platform.timers.threadx
        fwlib.utils
        fwlib.Driver.MboxIccTransport
        ms::driver::textio::pl011
        ms::lib::debug
        ms::lib::cortex::m7::nvic
        ms::lib::stdio_textio
        ms::lib::threadx
        ms::service::mcp_etc
        silibs::idhw
        silibs::idsw
        silibs::platform
        silibs::kng_silibs_platform
)

# Ensure ms::lib::arm_utils precedes ICC transport driver in linker invocation by specifying this link dependency edge
target_link_libraries(fwlib.Driver.MboxIccTransport PRIVATE ms::lib::arm_utils)
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

