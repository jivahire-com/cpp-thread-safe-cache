set(TARGET kng.service.sel_mgr)

add_library(${TARGET}
    STATIC
        sel_cli.c
        sel_dfwk.c
        sel_init.c
        sel_main.c
)

target_link_libraries(
    ${TARGET}
        PRIVATE
          fwlib.lib.pldm.protocol
          fwlib.service.cli
          fwlib.service.iccBase
          fwlib.service.pldm
          ms::lib::crash_dump
          ms::lib::event_trace::trace
          ms::lib::system_info
          ms::lib::utils
          ms::driver::icc_mhu_trans
          ms::driver::sensor_fifo
          silibs::icc_mhu
          silibs::kng_silibs_platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::service::sel_mgr ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
