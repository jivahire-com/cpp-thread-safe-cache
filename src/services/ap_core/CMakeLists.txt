# client interface
set(TARGET kng.client_interface.ap_core)

add_library(${TARGET}
    STATIC
    ap_core_client_interface.c
)

target_link_libraries(${TARGET}
    PUBLIC
    PRIVATE
        fwlib.dfwk
        fwlib.lib.interface.platform.errorhandler
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::client_interface::ap_core ALIAS kng.client_interface.ap_core)

# service 
set(TARGET kng.service.ap_core)

add_library(${TARGET}
    STATIC
        ${STARTUP_COMMON_SOURCE}
        ap_core.c
        ap_core_fw_load.c
        ap_core_ppu.c
        ap_core_util.c
)

target_link_libraries(${TARGET}
    PUBLIC
        ms::lib::corebits
    PRIVATE
        fwlib.dfwk
        fwlib.lib.cfg_mgr
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.init
        fwlib.service.iccBase
        ms::build_data
        ms::lib::accelerator_ip
        ms::lib::crash_dump
        ms::lib::debug
        ms::lib::error_codes
        ms::lib::hsp_firmware_headers
        ms::lib::system_info
        ms::lib::threadx
        ms::lib::utils
	    ms::lib::mesh
        ms::lib::bandgap
        ms::client_interface::ap_core
        ms::client_interface::startup_shutdown
        ms::service::fuse
        ms::service::pcie_manager
        ms::service::sds_service
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::pik_clock
        silibs::platform
        silibs::ppu_v1        
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::ap_core ALIAS kng.service.ap_core)

