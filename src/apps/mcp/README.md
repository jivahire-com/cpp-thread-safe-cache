# MCP Firmware

This directory contains all of the projects that are linked for the mcp firmware.

## src
   The src of the mcp application firmware
   This is the entrypoint of the firmware and links in privileged drivers.
   The app's main thread initializes all of the drivers.
## inc
   Common headers for the mcp across the app are stored here.
   This includes mcp_memory.ld, which is the main memory map describing the
   location and sizes of the mcp firmware
## binary
   This target combines the linked .elf files from the app to
   the output firmware .bin file to be loaded in TCM
