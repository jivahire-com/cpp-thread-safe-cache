# This target links the SCP boot loader firmware
#
#  Currently only the basic folder structure and compilation infrastructure is created
#
# The boot loader reuses the entry point from cortex M7 vectors. This will be updated
# once the complete boot loader reset flow is identified
#
# The boot loader's entry point is the "void main()" function, which will be invoked at the
# point that the C environment is ready.
#
# The boot loader is expected to zero initialize the ITCM and DTCM memories, uncompress the
# SCP firmware image that stored with the boot loader binary and switch to executing the firmware
# from ITCM once the decompression is completed.
#
# The file currently outputs only .elf file with plain bootloader contents.

set(TARGET scp_boot_loader)

add_executable(
    ${TARGET}
        main.c
        handlers.c
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            ms::vectors::m7_boot_loader
            boot_loader_interface
            register_headers
)

target_include_directories(
    ${TARGET}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../inc
)

target_link_options(
    ${TARGET}
        PRIVATE
            -T ${CMAKE_CURRENT_LIST_DIR}/../inc/scp_boot_loader_memory.ld
            -Wl,-Map=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scp_boot_loader/${TARGET}.map
            -L${CMAKE_CURRENT_SOURCE_DIR}/../inc
            -ereset_handler
            -Wl,--build-id
)

set_property(
    TARGET ${TARGET}
    APPEND PROPERTY LINK_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/../inc/scp_boot_loader_memory.ld
)

# Setting compiler optimization level to -Og to match Pioneer boot code compilation flags
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og")

# Generate a .bin file with the raw binary output from the linked .elf 
set(FIRMWARE_BIN ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scp_boot_loader/${TARGET}.bin)

add_custom_command(
    TARGET ${TARGET}
    POST_BUILD
    COMMAND ${OBJ_COPY} -O binary ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}${CMAKE_EXECUTABLE_SUFFIX_C} ${FIRMWARE_BIN}
    DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}
    BYPRODUCTS ${FIRMWARE_BIN}
    COMMENT "Creating scp_boot_loader bin file"
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
