set(TARGET kng.service.pwr_tlm_mcp.cmpnt.data_process)

add_library(${TARGET}
    STATIC
        compute_metrics.c
        data_proc_tlm_cmpnt.c
        data_sampling.c
        data_utilities.c
        die_2_die_exchange.c
        package_interface.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}

)

target_link_libraries(${TARGET}
    PUBLIC

    PRIVATE
        pwr_tlm_mcp.service.cmpnt.interfaces
        fwlib.lib.assert
        fwlib.lib.status
        fwlib.lib.utils
        ms::lib::error_codes
        ms::lib::power_fuse
        ms::lib::gtimer
        ms::lib::pwr_tlm_core_ex
        silibs::dvfs
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
