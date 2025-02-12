
set(TARGET in_band_telemetry_ddr)


if (i386-pc-windows-msvc)
    add_library(${TARGET}
        STATIC
            src/in_band_tlm_ddr.c
    )
    # allows unit test to override 64 bit DDR locations
    target_compile_definitions(${TARGET} PUBLIC IB_TLM_DDR_TEST_OVERRIDE=1)

    target_link_libraries(${TARGET}
        PUBLIC
            IFPFW.EventTracing
            ms::build_data
            ms::lib::utils
            ms::service::atu_service
    )

    target_include_directories(
        ${TARGET}
            PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}
    )

else()
    add_library(${TARGET} INTERFACE)

    target_link_libraries(${TARGET}
        INTERFACE
            IFPFW.EventTracing
            ms::build_data
            ms::lib::utils
            ms::service::atu_service
        )

    target_include_directories(
        ${TARGET}
            INTERFACE
                ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()




add_library(ms::lib::interface::in_band_telemetry_ddr ALIAS ${TARGET})
