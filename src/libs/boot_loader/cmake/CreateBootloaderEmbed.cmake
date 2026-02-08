#
# Copyright (c) Microsoft Corporation. All rights reserved.
#
# Generates the payload portion of the bootloader
#
# Creates build rules to:
#   * Split the main image .elf in to separate .elf.dtcm.bin, .elf.itcm.bin & .elf.rmss.bin files for the ITCM, DTCM & RMSS regions
#   * Compress the .bin files in to .bin.gz files
#   * Run a script to combine the .gz files into .elf.embed
#   * Copy the .embed file into the base bootloader
#   * Flatten the bootloader in to a .bin file
#   * Publish the bootloader
#
function(create_bootloader_embed FW_BLOCK FW_IMAGE_TARGET BOOT_LOADER_TARGET EMBED_FW_TARGET)

    # Path to the main image .elf file
    set(FW_IMAGE_PATH "$<TARGET_FILE:${FW_IMAGE_TARGET}>")
	
    set(OBJCOPY "$ENV{REPO_APP_PATH_gcc.arm.eabi.aarch-win64}/arm-none-eabi/bin/objcopy.exe")
    set(OBJDUMP "$ENV{REPO_APP_PATH_gcc.arm.eabi.aarch-win64}/arm-none-eabi/bin/objdump.exe")

    # Paths to the flat .bin files for the main image's initialization data
    # for the ITCM, DTCM & RMSS Data memories  
	
    set(ITCM_BIN_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FW_BLOCK}/${FW_IMAGE_TARGET}.elf.itcm.bin")
    set(DTCM_BIN_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FW_BLOCK}/${FW_IMAGE_TARGET}.elf.dtcm.bin")
    set(RMSS_BIN_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FW_BLOCK}/${FW_IMAGE_TARGET}.elf.rmss.bin")
    	
    # Paths to the compressed ITCM, DTCM & RMSS initialization files
    set(ITCM_BIN_GZ_PATH "${ITCM_BIN_PATH}.gz")
    set(DTCM_BIN_GZ_PATH "${DTCM_BIN_PATH}.gz")
    set(RMSS_BIN_GZ_PATH "${RMSS_BIN_PATH}.gz")

    # Path to the '.embed' file which includes the compressed ITCM, DTCM & RMSS data along
    # with a manifest header
    set(EMBED_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FW_BLOCK}/${FW_IMAGE_TARGET}.elf.embed")
    
    # Path to the "base" bootloader .elf file
    # The base bootloader is a baremetal target that does not yet contain the main image
    # payload. It is built in a separate cmake graph, and must be built before building
    # the firmware CMake project.
    set(BASE_BL_PATH "$<TARGET_FILE:${BOOT_LOADER_TARGET}>")

    # Path for the output bootloader .elf file
    # The output bootloader contains the compressed mainimage
    set(EMBED_FW_ELF_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EMBED_FW_TARGET}.elf")

    # Path for the output bootloader .bin file
    set(EMBED_FW_BIN_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FW_BLOCK}/${EMBED_FW_TARGET}.bin")
    
    # Path to the python script which generates the .embed file
    set(PYTHON_EMBED_SCRIPT "${CMAKE_SOURCE_DIR}/src/libs/boot_loader/scripts/BootLoaderEmbed.py")
	
    # Path to the python script which generates the .embed file
    set(PYTHON_GZ_SCRIPT "${CMAKE_SOURCE_DIR}/src/libs/boot_loader/scripts/CompressFiles.py")	
    
    find_package(Python3 COMPONENTS Interpreter)
    if (NOT ${Python3_FOUND})
        message(FATAL_ERROR "Python is needed to build embedded image.")
    endif()

    # Event trace metadata sections
    set(EVENT_TRACE_SECTIONS
        --only-section .EventMetadata.et.msdata
        --only-section .ProviderMetadata.et.msdata
        --only-section .ProviderMetadata.Test.et.msdata
        --only-section .EventMetadata.Test.et.msdata
    )

    # Decide which bin command gets the event trace sections
    set(ITCM_EXTRA_SECTIONS "")
    set(RMSS_EXTRA_SECTIONS "${EVENT_TRACE_SECTIONS}")

    if(FW_BLOCK STREQUAL "mcp")
        set(ITCM_EXTRA_SECTIONS "${EVENT_TRACE_SECTIONS}")
        set(RMSS_EXTRA_SECTIONS "")
    endif()

    add_custom_command(
        OUTPUT "${ITCM_BIN_PATH}"
        COMMAND ${OBJCOPY}
        ARGS -O binary 
            --only-section .vectors 
            --only-section .text 
            --only-section .rodata.itcm
            --only-section .BuildBinaryVersion
            --only-section .note.gnu.build-id
            ${ITCM_EXTRA_SECTIONS}
            "${FW_IMAGE_PATH}"
            "${ITCM_BIN_PATH}"
        DEPENDS "${FW_IMAGE_PATH}"
        COMMENT "Generating flat binary ${FW_IMAGE_TARGET}.itcm.bin"
    )

    add_custom_command(
        OUTPUT "${DTCM_BIN_PATH}"
        COMMAND ${OBJCOPY}
        ARGS -O binary 
            --only-section .data
            --only-section .data.fpfw_init
            --only-section .bss
            --only-section .heap
            --only-section .stack
            "${FW_IMAGE_PATH}"
            "${DTCM_BIN_PATH}"
        DEPENDS "${FW_IMAGE_PATH}"
        COMMENT "Generating flat binary ${FW_IMAGE_TARGET}.dtcm.bin"
    )

    add_custom_command(
        OUTPUT "${RMSS_BIN_PATH}"
        COMMAND ${OBJCOPY}
        ARGS -O binary 
            --only-section .rodata.rmss
            ${RMSS_EXTRA_SECTIONS}
            "${FW_IMAGE_PATH}"
            "${RMSS_BIN_PATH}"
        DEPENDS "${FW_IMAGE_PATH}"
        COMMENT "Generating flat binary ${FW_IMAGE_TARGET}.rmss.bin"
    )
    
    add_custom_target(${FW_IMAGE_TARGET}_itcm_bin ALL DEPENDS "${ITCM_BIN_PATH}")
    add_custom_target(${FW_IMAGE_TARGET}_dtcm_bin ALL DEPENDS "${DTCM_BIN_PATH}")
    add_custom_target(${FW_IMAGE_TARGET}_rmss_bin ALL DEPENDS "${RMSS_BIN_PATH}")
    
    add_custom_command(
        OUTPUT ${ITCM_BIN_GZ_PATH}
        COMMAND ${Python3_EXECUTABLE}
        ARGS ${PYTHON_GZ_SCRIPT}
            # Input files
            ${ITCM_BIN_PATH}
            # Output file
            ${ITCM_BIN_GZ_PATH}
        DEPENDS
            ${PYTHON_GZ_SCRIPT}
            ${ITCM_BIN_PATH}
        COMMENT "Compress ITCM into .gz ")     

    add_custom_command(
        OUTPUT ${DTCM_BIN_GZ_PATH}
        COMMAND ${Python3_EXECUTABLE}
        ARGS ${PYTHON_GZ_SCRIPT}
            # Input files
            ${DTCM_BIN_PATH}
            # Output file
            ${DTCM_BIN_GZ_PATH}
        DEPENDS
            ${PYTHON_GZ_SCRIPT}
            ${DTCM_BIN_PATH}
        COMMENT "Compress DTCM into .gz ") 
	
    add_custom_command(
        OUTPUT ${RMSS_BIN_GZ_PATH}
        COMMAND ${Python3_EXECUTABLE}
        ARGS ${PYTHON_GZ_SCRIPT}
            # Input files
            ${RMSS_BIN_PATH}
            # Output file
            ${RMSS_BIN_GZ_PATH}
        DEPENDS
            ${PYTHON_GZ_SCRIPT}
            ${RMSS_BIN_PATH}
        COMMENT "Compress RMSS RODATA into .gz ")

    add_custom_target(${FW_IMAGE_TARGET}_itcm_gz ALL DEPENDS ${ITCM_BIN_GZ_PATH} ${ITCM_BIN_GZ_PATH})
    add_custom_target(${FW_IMAGE_TARGET}_dtcm_gz ALL DEPENDS ${DTCM_BIN_GZ_PATH} ${DTCM_BIN_GZ_PATH})
    add_custom_target(${FW_IMAGE_TARGET}_rmss_gz ALL DEPENDS ${RMSS_BIN_GZ_PATH} ${RMSS_BIN_GZ_PATH})


    # Critical Size checks
    #   1. .itcm.bin must not exceed ITCM size (512kB)
    #   2. .dtcm.bin must not exceed DTCM size (512kB) minus
    #      space reserved for bootloader data
    #      The size of the bootloader data is inferred from the 
    #      size of the bootloader's .data, .bss, & .stackheap  sections
    #   3. The compressed images must fit within the bootloader in the boot RAM
    #      The size available in the bootloader is inferred from the size of the
    #      base bootloader's .mainimage section
    
    # The initialized ITCM data can use the entirety of the ITCM memory 
    math(EXPR ITCM_SIZE "524160")
    set(ITCM_BASE "0x000080")

    # The initialized DTCM data must reserve some space at the end of the DTCM
    # for the stack and heap
    set(DTCM_BASE "0x20000000")
    math(EXPR DTCM_SIZE "491520")

    # The RMSS ram region is separate for the SCP and MCP
    # SCP RMSS: 348KB (region shifted down 32KB from reclaimed WARM_START space)
    # MCP RMSS: 4KB (follows SCP RMSS)
    if(FW_BLOCK STREQUAL "scp")
        set(RMSS_BASE "0x01345000")
        math(EXPR RMSS_SIZE "454655")
    else()
        set(RMSS_BASE "0x013B4000")
        math(EXPR RMSS_SIZE "4096")
    endif()

    add_custom_command(
        OUTPUT "${EMBED_PATH}"
        COMMAND ${Python3_EXECUTABLE}
        ARGS ${PYTHON_EMBED_SCRIPT}
            # Input files
                ${ITCM_BIN_PATH}
                ${DTCM_BIN_PATH}
                ${RMSS_BIN_PATH}
                ${ITCM_BIN_GZ_PATH}
                ${DTCM_BIN_GZ_PATH}
                ${RMSS_BIN_GZ_PATH}
                ${BASE_BL_PATH}
                ${OBJDUMP}
            # Output file
                ${EMBED_PATH}
            # Size checks
                ${ITCM_BASE}
                ${ITCM_SIZE}
                ${DTCM_BASE}
                ${DTCM_SIZE}
                ${RMSS_BASE}
                ${RMSS_SIZE}
        DEPENDS
            ${PYTHON_EMBED_SCRIPT}
            ${ITCM_BIN_PATH}
            ${DTCM_BIN_PATH}
            ${RMSS_BIN_PATH}
            ${ITCM_BIN_GZ_PATH}
            ${DTCM_BIN_GZ_PATH}
            ${RMSS_BIN_GZ_PATH}
        COMMENT "Combining flat binaries into ${FW_IMAGE_TARGET}.embed")

    add_custom_target(${FW_IMAGE_TARGET}_embed DEPENDS ${EMBED_PATH})


    add_custom_command(
        OUTPUT ${EMBED_FW_ELF_PATH}
        COMMAND ${OBJCOPY}
        ARGS
            --update-section .mainimage=${EMBED_PATH}
            ${BASE_BL_PATH}
            ${EMBED_FW_ELF_PATH}
        DEPENDS
            ${EMBED_PATH}
            ${BASE_BL_PATH}
        COMMENT "Embedding ${FW_IMAGE_TARGET}.embed into bootloader")
        
    add_custom_target(${FW_IMAGE_TARGET}_bl DEPENDS ${EMBED_FW_ELF_PATH})

    add_custom_command(
        OUTPUT ${EMBED_FW_BIN_PATH}
        COMMAND ${OBJCOPY}
        ARGS
            -O binary
            ${EMBED_FW_ELF_PATH}
            ${EMBED_FW_BIN_PATH}
        DEPENDS
            ${EMBED_FW_ELF_PATH}
        COMMENT "Creating flat bootloader ${EMBED_FW_TARGET}.bin")
        
    add_custom_target(${FW_IMAGE_TARGET}_blbin ALL DEPENDS ${EMBED_FW_BIN_PATH})

    # The parent cmake project excludes targets from ALL by default
    # Set this target to be included so that a default build of the cmake graph 
    # will generate the bootloader output
    set_target_properties(
        ${FW_IMAGE_TARGET}_blbin
        PROPERTIES EXCLUDE_FROM_ALL FALSE
                   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

endfunction()
