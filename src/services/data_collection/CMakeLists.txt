set(TARGET kng.service.dcs)

add_library(${TARGET}
    STATIC
        src/data_collection_events.c
        src/data_collection_service.c
        src/dcp_service_client.c
        src/dcs_manifest.c
        src/tlm_relay.c
)

target_link_libraries(
    ${TARGET}
        PUBLIC
          fwlib.service.iccBase
          ms::lib::threadx
          silibs::idsw_kng
        PRIVATE
          fwlib.dfwk
          fwlib.platform.dfwkhost.threadx
          fwlib.lib.interface.platform.errorhandler
          ms::build_data
          ms::lib::interface::in_band_telemetry_ddr
          ms::lib::debug
          ms::lib::event_trace::trace
          silibs::kng_silibs_platform
          silibs::icc_mhu
          silibs::idhw
          silibs::idsw

)
target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(ms::service::dcs ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
