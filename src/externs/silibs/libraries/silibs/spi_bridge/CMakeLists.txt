message("----------------------------------")
message("---- Adding Silibs KNG PLAT ------")

set(TARGET extern.silibs.spi_bridge)

set(SOURCES ${TARGET_DIR}/libraries/spi_bridge/src/spi_bridge.c)

add_library(${TARGET} STATIC ${SOURCES})

target_include_directories(
    ${TARGET}
        SYSTEM PUBLIC
            ${TARGET_DIR}/libraries/spi_bridge/include
        SYSTEM PRIVATE
            ${TARGET_DIR}/libraries/spi_bridge/private_include
)

target_link_libraries(${TARGET}
    PUBLIC
        silibs::platform
        register_headers
)

add_library(spi_bridge ALIAS ${TARGET})
add_library(silibs::spi_bridge ALIAS ${TARGET})
