enabled_on(arm-eabi-aarch)

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
        uart_scp_init.c
)

target_link_libraries(
    ${TARGET} 
    PRIVATE
        fwlib.dfwk
        fwlib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.utils
        ms::driver::bm::uart
        ms::lib::css
        ms::lib::debug
        ms::lib::cortex::m7::nvic
        ms::lib::mesh
        ms::lib::threadx
        ms::service::scp_etc
        silibs::idhw
        silibs::idsw
        silibs::platform
        silibs::kng_silibs_platform
)

lint_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::runtime_init::scp ALIAS ${TARGET})

set(TARGET kng.lib.scp.runtime_init.mcp)

add_library(${TARGET} 
    OBJECT 
        debug_init.c
        dfwk_init.c
        etc_mcp_init.c
        nvic_init.c
        uart_mcp_init.c
)

target_link_libraries(
    ${TARGET} 
    PRIVATE
        fwlib.dfwk
        fwlib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.utils
        ms::driver::bm::uart
        ms::lib::debug
        ms::lib::cortex::m7::nvic
        ms::lib::threadx
        ms::service::mcp_etc
        silibs::idhw
        silibs::idsw
        silibs::platform
        silibs::kng_silibs_platform
)

lint_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::runtime_init::mcp ALIAS ${TARGET})

