#
# PLDM Driver
#
set(TARGET kng.driver.pldm)

add_library(${TARGET}
    STATIC
    pldm_drv.c
)

target_include_directories(${TARGET}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PUBLIC
        fwlib.dfwk
        ms::lib::error_codes
        silibs_platform
    PRIVATE
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.pldm.protocol
        fwlib.service.pldm
        ms::lib::crash_dump
        silibs::kng_silibs_platform
)

add_library(ms::driver::pldm ALIAS kng.driver.pldm)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
