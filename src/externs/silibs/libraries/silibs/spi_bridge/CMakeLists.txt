message("----------------------------------")
message("---- Adding Silibs SPI Bridge ----")

set(TARGET extern.silibs.spi_bridge)

set(SRCS
    ${TARGET_DIR}/libraries/spi_bridge/src/spi_bridge.c
    ${TARGET_DIR}/libraries/spi_bridge/src/mscp_exp_spi_synchronize_dies.c
)

add_library(${TARGET} STATIC ${SRCS})

target_include_directories(
    ${TARGET}
        SYSTEM PUBLIC
            ${TARGET_DIR}/libraries/spi_bridge/include
        SYSTEM PRIVATE
            ${TARGET_DIR}/libraries/spi_bridge/private_include
)

target_link_libraries(${TARGET}
    PUBLIC
        silibs::kng_silibs_platform 
)

add_library(spi_bridge ALIAS ${TARGET})
add_library(silibs::spi_bridge ALIAS ${TARGET})
