
# This target builds the complete MCP firmware binary package
#
# This target combines the .bin files of the application firmware and other files as needed
# in to a single .bin file that can be loaded in to the start address of TCM

# Get the paths to the application firmware and other binaries
get_target_property(MCP_FW_BIN mcp_fw FIRMWARE_BIN)

set(BIN_INPUTS 
    ${MCP_FW_BIN}
)

set(BIN_OUTPUT
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mcp/mcp_fw_pkg.bin)

# Use the cmake "cat" command to combine the binary files
add_custom_command(
    OUTPUT ${BIN_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E cat ${BIN_INPUTS} > ${BIN_OUTPUT}
    DEPENDS ${BIN_INPUTS}
    COMMENT "Creating firmware bin")

# Define a custom target that depends on the output of this command
add_custom_target(
    mcp_firmware ALL
    DEPENDS ${BIN_OUTPUT})

# Set the FIRMWARE_BIN property for this target such that other targets
# can locate the output if needed (for test/packaging steps)
set_target_properties(
    mcp_firmware 
    PROPERTIES 
    FIRMWARE_BIN ${BIN_OUTPUT})

# Ensure that the firmware is built after the application firmware and other binaries
add_dependencies(
    mcp_firmware
        mcp_fw
)
