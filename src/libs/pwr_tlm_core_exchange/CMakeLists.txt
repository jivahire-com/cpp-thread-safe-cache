set(TARGET kng.lib.pwr_tlm_core_ex)

add_library(${TARGET}
    STATIC
        pwr_tlm_core_exchange.c
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.utils
        ms::cmsis::interface

    PUBLIC
        silibs::kng_silibs_platform
        silibs::idsw_kng
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

if (i386-pc-windows-msvc)
    # allows unit test to override product memory locations
    target_compile_definitions(${TARGET} PUBLIC PWR_TLM_EXCH_TEST_OVERRIDE=1)
endif()

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::pwr_tlm_core_ex ALIAS ${TARGET})
