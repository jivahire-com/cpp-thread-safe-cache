set(TARGET scp_avs_cli)

set(SRC
    ./scp_avs_cli.c)

add_library(${TARGET} STATIC ${SRC})

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            fwlib.service.cli
            silibs::avs
            silibs::idsw_kng
            scp_avs
        PUBLIC
            fwlib.lib.assert
            fwlib.platform.dfwkhost.threadx
            fwlib.lib.interface.platform.threadx
            fwlib.lib.utils
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::scp_avs::cli ALIAS ${TARGET})
