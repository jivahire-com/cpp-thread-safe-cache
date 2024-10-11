set(TARGET accel_mbox_icc_transport)

set (SRCS
    src/accel_mbox_icc_transport.c
)

add_library(${TARGET} STATIC ${SRCS})

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc)

target_link_libraries(${TARGET}
    PUBLIC
        fwlib.dfwk
        fwlib.lib.interface.platform.interrupts
        fwlib.lib.interface.platform.timers
        fwlib.driver.interface.iccTransport
        fwlib.lib.mboxPrimitives
    PRIVATE
        fwlib.lib.assert)

lint_target(${TARGET})
format_target(${TARGET})
# Coverage is not needed for this library as it has been temporarily added to fix ICC stack limitation

add_library(ms::lib::accel_mbox_icc_transport ALIAS ${TARGET})
