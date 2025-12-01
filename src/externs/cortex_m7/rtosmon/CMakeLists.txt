
set(TARGET kng.service.platform.m7.rtosmon)


add_library(${TARGET}
    STATIC
    rtosmon_config.c
)

target_link_libraries(
    ${TARGET}
        PUBLIC
        PRIVATE
          fwlib.lib.interface.platform.errorhandler
          fwlib.lib.interface.platform.specialization.mts
          fwlib.lib.status
          ms::build_data
          ms::lib::event_trace::trace
          ms::lib::icc_platform_defines
          ms::lib::interface::in_band_telemetry_ddr
          silibs::icc_mhu
          silibs::idhw
          silibs::idsw
          silibs::idsw_kng
          silibs::kng_silibs_platform
          ms::cmsis::interface
          fwlib.lib.rtosmon
          ms::lib::crash_dump
          ms::lib::systick_update
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Give Platform specialization access to private includes
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:fwlib.service.mts,INCLUDE_DIRECTORIES>)

add_library(ms::service::m7_rtosmon ALIAS kng.service.platform.m7.rtosmon)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
