set(TARGET kng.lib.vab)

add_library(${TARGET}
    STATIC
        vab_common_setup.c
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            fwlib.lib.assert
            ms::lib::debug
            silibs::atu
            silibs::kng_silibs_platform
            silibs::platform
            silibs::smmu
            silibs::vab_init
            silibs_platform

)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::lib::vab ALIAS kng.lib.vab)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
