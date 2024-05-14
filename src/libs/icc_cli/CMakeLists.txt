set(TARGET icc_cli)

set(SRC
    src/icc_mbox_cli.c
)

set(LIBS 
        PRIVATE
            fwlib.cli
            fwlib.utils
            silibs::kng_silibs_platform
            silibs::platform
)

add_library(${TARGET} STATIC ${SRC})
target_link_libraries(${TARGET} ${LIBS})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

lint_target(${TARGET})
format_target(${TARGET})

add_library(ms::lib::icc::cli ALIAS ${TARGET})
