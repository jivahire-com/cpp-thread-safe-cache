# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.service.sensor_fifo)

add_library(${TARGET}
    STATIC
        sensor_fifo_common.c
        sensor_fifo_events.c
        sensor_fifo_public.c
)

target_link_libraries(
    ${TARGET}
        PRIVATE
          fwlib.dfwk
          fwlib.platform.dfwkhost.threadx
          fwlib.lib.interface.platform.errorhandler
          fwlib.service.iccBase
          ms::lib::debug
          ms::lib::threadx
          ms::lib::event_trace::trace
          ms::driver::icc_mhu_trans
          ms::driver::sensor_fifo
          silibs::kng_silibs_platform

)

# Many of the fifo's are populated by hardware, therefore the entry format is pre-determined.
# telemetry_data_struct.h in silibs::sensor_ram_bridge defines the hardware telemetry formats.
# Want to avoid duplication/syncronization of this header, but also don't want the service layer to have
# access to the silibs API's which should be limited to the driver.
# Copy this header to it's own directory, and make that single header file publicly available to consumers.
# telemetry_data_struct.h is included in sensor_fifo_service.h
get_target_property(ram_bridge_dir silibs::sensor_ram_bridge INTERFACE_INCLUDE_DIRECTORIES)
add_custom_target(
    sensor.fifo.include
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/sensor_fifo_include
    COMMAND PowerShell Copy-Item -Path '${ram_bridge_dir}/telemetry_data_struct.h' -Destination '${CMAKE_CURRENT_BINARY_DIR}/sensor_fifo_include' -Force;
    DEPENDS ${ram_bridge_dir}/telemetry_data_struct.h
)

add_dependencies(${TARGET} sensor.fifo.include)
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${CMAKE_CURRENT_BINARY_DIR}/sensor_fifo_include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::service::sensor_fifo ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
