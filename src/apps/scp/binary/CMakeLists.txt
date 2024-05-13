
# This target builds the complete SCP firmware binary package
#
# This target combines the .bin files of the application firmware and other files as needed
# in to a single .bin file that can be loaded in to the start address of TCM

# Get the paths to the application firmware and other binaries
# Also it takes bootloader and runtime to combine them into embedded elf and bin , with runtime 
# firmware ITCM and DTCM sections embedded as compressed .gz , prepended by header.
include("${CMAKE_SOURCE_DIR}/src/libs/boot_loader/cmake/CreateBootloaderEmbed.cmake")

get_target_property(SCP_FW_BIN scp_fw FIRMWARE_BIN)

set(BIN_INPUTS 
    ${SCP_FW_BIN}
)

set(BIN_OUTPUT
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scp/scp_fw_pkg.bin)

# Use the cmake "cat" command to combine the binary files
add_custom_command(
    OUTPUT ${BIN_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E cat ${BIN_INPUTS} > ${BIN_OUTPUT}
    DEPENDS ${BIN_INPUTS}
    COMMENT "Creating firmware bin")

# Define a custom target that depends on the output of this command
add_custom_target(
    scp_firmware ALL
    DEPENDS ${BIN_OUTPUT})

# Set the FIRMWARE_BIN property for this target such that other targets
# can locate the output if needed (for test/packaging steps)
set_target_properties(
    scp_firmware 
    PROPERTIES 
    FIRMWARE_BIN ${BIN_OUTPUT})

# Ensure that the firmware is built after the application firmware and other binaries
add_dependencies(
    scp_firmware
        scp_fw
        scp_boot_loader
)

# Embeds  the bootloader and fw into a single elf and also generate .bin file
create_bootloader_embed(scp scp_fw scp_boot_loader scp_boot_loader_embed_fw)