# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET scp_avs)

set(SRC
    ./scp_avs_driver.c
    ./scp_avs_api.c
    ./scp_avs_events.c
)

set(LIBS PUBLIC fwlib.dfwk fwlib.lib.linkedlist fwlib.lib.assert)

add_library(${TARGET} STATIC ${SRC})

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            silibs::avs
            ms::lib::crash_dump
            ms::lib::arm_intrinsic
            ms::lib::cortex_m7::nvic
            ms::service::power
            silibs::ddrss
            scp_avs_cli
        PUBLIC
            ms::lib::debug
            fwlib.dfwk
            fwlib.lib.init
            fwlib.lib.linkedlist
            fwlib.lib.utils
            silibs::dvfs
            silibs::kng_silibs_platform
            silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::scp_avs ALIAS ${TARGET})
