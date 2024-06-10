# This target links the SCP application firmware
#
# The actual entrypoint to the program will be defined in the vectors library
# see vectors for details on how the initial CPU and C environment are initialized
#
# The firmware's entry point is the "void main()" function, which will be invoked at the
# point that the C environment is ready.
#
# The firmware will initialize the ThreadX environment and initialize any drivers linked
# in to it.
#
# The firmware is linked to run from a fixed address. The address the firmware is linked at
# are found in scp_memory.ld
#
# The output of this target is a .elf and .bin file for the application firmware

set(TARGET scp_fw)

add_executable(
    ${TARGET}
        main.c
        scp_events.c
)

# Set output location
set_target_properties( ${TARGET}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scp"
)


# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_link_libraries(
    ${TARGET}
        PRIVATE
            ms::lib::event_trace::trace
            ms::lib::threadx
            ms::lib::stdio_textio
            ms::driver::textio::pl011
            ms::vectors::cortex_m7
            ms::lib::runtime_init::scp
            fwlib.lib.init
            fwlib.lib.utils

)

target_include_directories(
    ${TARGET}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../inc
)

target_link_options(
    ${TARGET}
        PRIVATE
            -T ${CMAKE_CURRENT_LIST_DIR}/../inc/scp_memory.ld
            -Wl,-Map=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scp/${TARGET}.map
            -L${CMAKE_CURRENT_SOURCE_DIR}/../inc
            -ereset_handler
            -Wl,--build-id
)

set_property(
    TARGET ${TARGET}
    APPEND PROPERTY LINK_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/../inc/scp_memory.ld
        ${COMMON_LINK_FILES}
)

# By setting the THREADX_KERNEL property
#   targets that have different linking requirements between kernel and other setups
#   can use generator expressions to add additional link time requiements to this
#   target. See io/CMakeLists.txt for an example.
set_target_properties(
    ${TARGET}
        PROPERTIES
            THREADX_KERNEL 1
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

# Embeds  the bootloader and fw into a single elf and also generate .bin file
include("${CMAKE_SOURCE_DIR}/src/libs/boot_loader/cmake/CreateBootloaderEmbed.cmake")
create_bootloader_embed(scp scp_fw scp_boot_loader scp_boot_loader_embed_fw)
