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

# Set output location
set_target_properties( ${TARGET}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scp_boot_loader"
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            ms::vectors::m7_boot_loader
            ms::boot_loader::interface
            ms::lib::boot_loader
            ms::cmsis::interface
            ms::lib::cortex_m7::mpu
            register_headers
)

target_include_directories(
    ${TARGET}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../inc
            ${REPO_ROOT_DIR}/.externs/repo/cmsis/CMSIS/Core/Include
            ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include/memory_map
            ${REPO_ROOT_DIR}/.externs/repo/silibs/fpga_enablement/baremetal/include

)

add_compile_definitions(BL_REL_STR="${REPO_ROOT_DIR}/${CMAKE_BUILD_TYPE}")

target_link_options(
    ${TARGET}
        PRIVATE
            -T ${CMAKE_CURRENT_LIST_DIR}/../inc/scp_boot_loader_memory.ld
            -Wl,-Map=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scp_boot_loader/${TARGET}.map
            -L${CMAKE_CURRENT_SOURCE_DIR}/../inc
            -ereset_handler
            -Wl,--build-id
)

# The below linker changes are from Pioneer, they're used to remove linker errors with below functions
target_link_options(${TARGET} PUBLIC "LINKER:--undefined=_open"
                                        "LINKER:--undefined=_kill"
                                        "LINKER:--undefined=_getpid"
                                        "LINKER:--undefined=_write"
                                        "LINKER:--undefined=_close"
                                        "LINKER:--undefined=_fstat"
                                        "LINKER:--undefined=_isatty"
                                        "LINKER:--undefined=_lseek"
                                        "LINKER:--undefined=_read"
                                        "-Og")


# The libnosys library provides stub definitions for syscalls needed to prevent linker errors
target_link_options(${TARGET} PUBLIC "--specs=nosys.specs")

set_property(
    TARGET ${TARGET}
    APPEND PROPERTY LINK_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/../inc/scp_boot_loader_memory.ld
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

# Setting compiler optimization level to -Og to match Pioneer boot code compilation flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
