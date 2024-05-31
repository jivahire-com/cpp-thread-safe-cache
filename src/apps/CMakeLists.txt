enabled_on(arm-eabi-aarch)

# Defining platform APIs
set(FPFW_PLATFORM_IMPL_THREADX "extern.threadx.cortex_m7")

add_subdirectory(mcp)
add_subdirectory(scp)
add_subdirectory(scp_boot_loader)
add_subdirectory(mcp_boot_loader)

# Trace manifest
set(MANIFEST_TARGET event_trace_manifest)
set(MANIFEST_FILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/man/etman.bin)

add_custom_command(
    OUTPUT ${MANIFEST_FILE}
    COMMAND $ENV{REPO_APP_PATH_python.win64}/tools/python.exe -m diagtool.app ${MANIFEST_FILE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scp/scp_fw.elf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mcp/mcp_fw.elf
    DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scp/scp_fw.elf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mcp/mcp_fw.elf
    COMMENT "Generating Trace Manifest - ${MANIFEST_FILE}"
)

add_custom_target(${MANIFEST_TARGET} ALL
    DEPENDS ${MANIFEST_FILE}
)
