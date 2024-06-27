# the driver is separated into two separate cmake libs
#  1. For future code re-use, a simple cmake variable could be set to select a variant of the device
#  2. This ensures, there are no silicon dependencies in sensor_fifo_driver_inf which ensures it is portable to
#     future devices

set(TARGET_DRIVER kng.driver.sensor_fifo)

add_library(${TARGET_DRIVER}
    STATIC
    sensor_fifo_driver_inf.c
)

target_link_libraries(${TARGET_DRIVER}
    PUBLIC
    fwlib.lib.status

    PRIVATE
    fwlib.dfwk
    fwlib.platform.dfwkhost.threadx
    fwlib.lib.assert
    fwlib.lib.interface.platform.errorhandler
    fwlib.lib.utils
    kng.device.scf_mhu
    ms::lib::threadx
)

target_include_directories(${TARGET_DRIVER}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::driver::sensor_fifo ALIAS kng.driver.sensor_fifo)

lint_target(${TARGET_DRIVER})
format_target(${TARGET_DRIVER})
coverage_target(${TARGET_DRIVER})


set(TARGET_DEVICE kng.device.scf_mhu)

add_library(${TARGET_DEVICE}
    STATIC
    scf_mhu_device.c
)

target_link_libraries(${TARGET_DEVICE}
    PUBLIC
    fwlib.lib.status

    PRIVATE
    fwlib.lib.assert
    fwlib.dfwk
    fwlib.lib.interface.platform.errorhandler
    fwlib.platform.dfwkhost.threadx
    fwlib.lib.utils
    kng_silibs_platform
    ms::lib::threadx
    silibs_platform
    silibs::sensor_ram_bridge
)

target_include_directories(${TARGET_DEVICE}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

lint_target(${TARGET_DEVICE})
format_target(${TARGET_DEVICE})
coverage_target(${TARGET_DEVICE})
