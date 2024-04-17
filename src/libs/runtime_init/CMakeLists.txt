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
        mesh_init.c
        nvic_init.c
        textio_init.c
)

target_link_libraries(
    ${TARGET} 
    PRIVATE
        fwlib.dfwk
        fwlib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.utils
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

target_compile_definitions(${TARGET} PRIVATE UART_BASE_ADDR=SCP_TOP_SCP_UART_ADDRESS)

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
        nvic_init.c
        textio_init.c
)

target_link_libraries(
    ${TARGET} 
    PRIVATE
        fwlib.dfwk
        fwlib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.utils
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

target_compile_definitions(${TARGET} PRIVATE UART_BASE_ADDR=MCP_TOP_MCP_UART_ADDRESS)

lint_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::runtime_init::mcp ALIAS ${TARGET})

