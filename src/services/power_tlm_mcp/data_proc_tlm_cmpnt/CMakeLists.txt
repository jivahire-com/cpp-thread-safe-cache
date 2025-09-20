set(TARGET kng.service.pwr_tlm_mcp.cmpnt.data_process)

add_library(${TARGET}
    STATIC
        aging_counters.c
        compute_metrics.c
        data_proc_tlm_cmpnt.c
        data_sampling.c
        data_utilities.c
        die_2_die_exchange.c
        in_band_package_interface.c
        out_of_band_interface.c
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
        ms::cmsis::interface
        ms::lib::arm_intrinsic
        ms::lib::core_info
        ms::lib::error_codes
        ms::lib::event_trace::trace
        ms::lib::gtimer
        ms::lib::pwr_tlm_core_ex
        ms::lib::telemetry::fuses
        silibs::dvfs
        silibs::pvt
        silibs::semaphore
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
