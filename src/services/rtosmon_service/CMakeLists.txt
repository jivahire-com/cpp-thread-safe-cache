
set(TARGET rtosmon_service)

add_library(${TARGET}
    STATIC
    rtosmon_service.c
)

target_link_libraries(
    ${TARGET}
        PUBLIC
            fwlib.lib.status
            ms::lib::threadx
        PRIVATE
            fwlib.lib.init
            fwlib.lib.interface.platform.errorhandler
            fwlib.lib.interface.platform.specialization.mts
            fwlib.lib.rtosmon
            ms::build_data
            ms::cmsis::interface
            ms::lib::crash_dump
            ms::lib::event_trace::trace
            ms::lib::gtimer
            ms::lib::icc_platform_defines
            ms::lib::interface::in_band_telemetry_ddr
            ms::lib::systick_update
            silibs::icc_mhu
            silibs::idhw
            silibs::idsw
            silibs::idsw_kng
            silibs::kng_silibs_platform
            silibs::platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

add_library(ms::service::m7_rtosmon_service ALIAS rtosmon_service)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
