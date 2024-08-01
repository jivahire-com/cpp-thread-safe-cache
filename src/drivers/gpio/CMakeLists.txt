#
# GPIO Driver
#
set(TARGET kng.driver.gpio)

add_library(${TARGET}
    STATIC
    gpio.c
)

target_include_directories(${TARGET}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PUBLIC
    fwlib.dfwk
    fwlib.platform.dfwkhost.threadx
    ms::lib::error_codes
    ms::lib::threadx
    silibs::gpio
    silibs_platform
    PRIVATE
    fwlib.lib.assert
    fwlib.lib.init
    fwlib.lib.utils
    silibs::kng_silibs_platform
    ms::lib::cortex_m7::nvic
)

add_library(ms::driver::gpio ALIAS kng.driver.gpio)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
