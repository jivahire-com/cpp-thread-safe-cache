# This target links the mcp application firmware
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
# are found in mcp_memory.ld
#
# The output of this target is a .elf and .bin file for the application firmware

set(TARGET mcp_fw)

add_executable(
    ${TARGET}
        main.c
        mcp_events.c
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(
    ${TARGET}
        PRIVATE
            ms::lib::debug
            ms::lib::event_trace::trace
            ms::lib::threadx
            ms::service::mcp_etc
            ms::driver::uart
            ms::vectors::cortex_m7
            fwlib.platform.dfwkhost.threadx
            fwlib.dfwk
)

target_include_directories(
    ${TARGET}
        SYSTEM PUBLIC
            ${REPO_ROOT_DIR}/.externs/azpkg/kng_soc_mason_csr_collateral.0.5.1/ap_addr_map/registerheader
            ${REPO_ROOT_DIR}/.externs/azpkg/kng_soc_mason_csr_collateral.0.5.1/mcp_addr_map/registerheader

        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../inc
)

target_link_options(
    ${TARGET}
        PRIVATE 
            -T ${CMAKE_CURRENT_LIST_DIR}/../inc/mcp_memory.ld 
            -Wl,-Map=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mcp/${TARGET}.map 
            -L${CMAKE_CURRENT_SOURCE_DIR}/../inc
            -ereset_handler
)

set_property(
    TARGET ${TARGET}
    APPEND PROPERTY LINK_DEPENDS 
        ${CMAKE_CURRENT_LIST_DIR}/../inc/mcp_memory.ld 
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

# Generate a .bin file with the raw binary output from the linked .elf 
set(FIRMWARE_BIN ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mcp/${TARGET}.bin)

add_custom_command(
    TARGET ${TARGET}
    POST_BUILD
    COMMAND ${OBJ_COPY} -O binary ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}${CMAKE_EXECUTABLE_SUFFIX_C} ${FIRMWARE_BIN}
    DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}
    BYPRODUCTS ${FIRMWARE_BIN}
    COMMENT "Creating mcp_fw bin file"
)

# Set the FIRMWARE_BIN property on this target
# This allows other targets to find the path to the .bin file for combining in to the complete firmware package
# see package/CMakeLists.txt
set_property(
    TARGET ${TARGET} 
    PROPERTY FIRMWARE_BIN
        ${FIRMWARE_BIN}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
