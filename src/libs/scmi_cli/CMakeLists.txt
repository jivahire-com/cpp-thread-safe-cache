set(TARGET scmi_cli)

set(SRC
    src/scmi_cli.c
)

set(LIBS
        PRIVATE
            fwlib.service.cli
            fwlib.lib.utils
            silibs::platform
            ms::driver::scmi_drv
            ms::lib::arm_intrinsic
            ms::lib::register_headers			
        PUBLIC
            ms::lib::debug
            silibs_platform
)

add_library(${TARGET} STATIC ${SRC})
target_link_libraries(${TARGET} ${LIBS})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

lint_target(${TARGET})
format_target(${TARGET})

add_library(ms::lib::scmi::cli ALIAS ${TARGET})
