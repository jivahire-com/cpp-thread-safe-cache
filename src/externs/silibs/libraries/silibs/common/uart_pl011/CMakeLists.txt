message("----------------------------------")
message("---- Adding Silibs UART PL011 ----")

set(TARGET extern.silibs.common.uart_pl011)

set(SRCS
    ${TARGET_DIR}/libraries/common/uart_pl011/src/uart_pl011.c
)

add_library(${TARGET} STATIC ${SRCS})

target_include_directories(
    ${TARGET}
        SYSTEM PUBLIC
            ${TARGET_DIR}/libraries/common/uart_pl011/include
)

target_link_libraries(${TARGET}
    PUBLIC
        silibs::platform
)

add_library(silibs::common::lib_uart_pl011 ALIAS ${TARGET})