# The purpose of this make file is to build the boot loader vector and crt0 files
# They provide the reset vector and _start implementations

enabled_on(arm-eabi-aarch)

set(TARGET vectors.m7_boot_loader)

set(SRC
    boot_loader_vectors.S
    boot_loader_crt0.S
)

add_library(${TARGET} STATIC ${SRC})

# Linting and format skipped as we have .S files

add_library(ms::vectors::m7_boot_loader ALIAS vectors.m7_boot_loader)