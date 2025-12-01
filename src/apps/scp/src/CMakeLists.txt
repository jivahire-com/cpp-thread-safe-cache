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
            ms::lib::newlib_lock
            ms::lib::event_trace::trace
            ms::lib::threadx
            ms::lib::stdio_textio
            ms::driver::textio::pl011
            ms::vectors::cortex_m7
            ms::lib::runtime_init::scp
            fwlib.lib.init
            fwlib.lib.utils
            ms::lib::system_info
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
            -L${CMAKE_BINARY_DIR}/ld/scp
            -ereset_handler
            -Wl,--build-id
            -Wl,--undefined=__retarget_lock_init
            -Wl,--undefined=__retarget_lock_acquire
            -Wl,--undefined=__retarget_lock_try_acquire
            -Wl,--undefined=__retarget_lock_release
            -Wl,--undefined=__retarget_lock_close
            -Wl,--undefined=__retarget_lock_init_recursive
            -Wl,--undefined=__retarget_lock_acquire_recursive
            -Wl,--undefined=__retarget_lock_try_acquire_recursive
            -Wl,--undefined=__retarget_lock_release_recursive
            -Wl,--undefined=__retarget_lock_close_recursive
)

# Set rodata memory region
set(RODATA_REGION RMSS_DATA)
set(RODATA_SECTION .rodata.rmss)
set(LD_SCRIPT_IN "${CMAKE_SOURCE_DIR}/tools/cmakes/toolchain/arm-eabi-aarch/ld/kernel.ld.in")
set(LD_SCRIPT_OUT "${CMAKE_BINARY_DIR}/ld/scp/kernel.ld")

configure_file(
    ${LD_SCRIPT_IN}
    ${LD_SCRIPT_OUT}
    @ONLY
)

add_custom_target(SCP_LINKER_SCRIPT ALL DEPENDS ${LD_SCRIPT_OUT})
add_dependencies(${TARGET} SCP_LINKER_SCRIPT)

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

add_custom_command(
    TARGET ${TARGET}
    COMMAND init_graph_tool -i ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scp/scp_fw.elf
    COMMAND echo "Checking for Cyclic Dependencies for ${TARGET}"
)

add_custom_command(
    TARGET ${TARGET}
    COMMAND $ENV{REPO_APP_PATH_gcc.arm.eabi.aarch-win64}/bin/arm-none-eabi-objdump.exe -xdlSC ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scp/scp_fw.elf > ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scp/scp.dis
    COMMAND echo "Generating scp.dis ${TARGET}"
)