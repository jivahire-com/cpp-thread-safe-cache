//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file kingsgate_boot.c
 * This file contains the load_image function for the MCP/SCP boot loader. It invokes unpack_image
 * API to decompress the MCP/SCP FW to the TCMs and return ITCM base address on success and NULL
 * on failure.
 */

#include <kingsgate_boot.h>
#include <post_codes.h>
#include <stddef.h>
#include <unpack_image.h>

/*-- Symbolic Constant Macros (defines) --*/

// Set variable 'boot_status' with statusCode and leave if
// condition is true.
#define SET_LEAVE_IF(condition, statusCode, label) \
    if (condition)                                 \
    {                                              \
        boot_status = statusCode;                  \
        goto label;                                \
    }

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

/*
 * TODO: Need to add logic to send boot trace events to HSP
 * ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484373
 */
/**
 *  @brief Function that parses the boot config and invokes decompress
 *         APIs to load firmware into TCMs.
 *
 *
 *  @param[in] boot_config  Structure holding the boot configuration
 *
 *  @return
 *      Returns NULL if decompress or boot config validation failed else base address of ITCM from boot config
 */
void* load_image(kingsgate_boot_config_t* boot_config)
{
    uint32_t boot_status = POST_CODE_SCP_OK;
    uint32_t image_size = 0;
    kingsgate_boot_metadata_t* boot_meta_data = NULL;
    uint32_t is_warm_boot_detected = 0;

    if (boot_config == NULL)
    {
        goto load_image_failed;
    }

    image_size = (uint32_t)(boot_config->data_src_end - boot_config->data_src_base) + 1;

    if (boot_config->cpu_type == MSCP_CPU_MCP)
    {
        // TODO: Send post a code indicating SCP/MCP has started
        // ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484373

        SET_LEAVE_IF(boot_config->data_src_base == 0, KINGSGATE_BOOT_POSTCODE_E_DATA_SRC_BASE_MCP, load_image_failed);
        SET_LEAVE_IF(boot_config->data_src_end <= boot_config->data_src_base,
                     KINGSGATE_BOOT_POSTCODE_E_IMAGE_SIZE_SCP,
                     load_image_failed);
        SET_LEAVE_IF(boot_config->itc_ram_base == 0, KINGSGATE_BOOT_POSTCODE_E_ITCRAM_BASE_MCP, load_image_failed);
        SET_LEAVE_IF(boot_config->itc_ram_size == 0, KINGSGATE_BOOT_POSTCODE_E_ITCRAM_SIZE_MCP, load_image_failed);
        SET_LEAVE_IF(boot_config->dtc_ram_base == 0, KINGSGATE_BOOT_POSTCODE_E_DTCRAM_BASE_MCP, load_image_failed);
        SET_LEAVE_IF(boot_config->dtc_ram_size == 0, KINGSGATE_BOOT_POSTCODE_E_DTCRAM_SIZE_MCP, load_image_failed);
        SET_LEAVE_IF(boot_config->boot_meta_base == 0, KINGSGATE_BOOT_POSTCODE_E_BOOT_META_MCP, load_image_failed);
    }
    else if (boot_config->cpu_type == MSCP_CPU_SCP)
    {
        // TODO: Send post a code indicating SCP/MCP has started
        // ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484373

        SET_LEAVE_IF(boot_config->data_src_base == 0, KINGSGATE_BOOT_POSTCODE_E_DATA_SRC_BASE_SCP, load_image_failed);
        SET_LEAVE_IF(boot_config->data_src_end <= boot_config->data_src_base,
                     KINGSGATE_BOOT_POSTCODE_E_IMAGE_SIZE_SCP,
                     load_image_failed);
        SET_LEAVE_IF(boot_config->itc_ram_base == 0, KINGSGATE_BOOT_POSTCODE_E_ITCRAM_BASE_SCP, load_image_failed);
        SET_LEAVE_IF(boot_config->itc_ram_size == 0, KINGSGATE_BOOT_POSTCODE_E_ITCRAM_SIZE_SCP, load_image_failed);
        SET_LEAVE_IF(boot_config->dtc_ram_base == 0, KINGSGATE_BOOT_POSTCODE_E_DTCRAM_BASE_SCP, load_image_failed);
        SET_LEAVE_IF(boot_config->dtc_ram_size == 0, KINGSGATE_BOOT_POSTCODE_E_DTCRAM_SIZE_SCP, load_image_failed);
        SET_LEAVE_IF(boot_config->boot_meta_base == 0, KINGSGATE_BOOT_POSTCODE_E_BOOT_META_SCP, load_image_failed);
    }
    else
    {
        // TODO: Send post a code indicating error in boot
        // ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484373
        goto load_image_failed;
    }

    // Boot metadata is expected to be stored at the beginning of thxe MSCP_EXP SRAM chosen
    boot_meta_data = (kingsgate_boot_metadata_t*)(boot_config->boot_meta_base);
    is_warm_boot_detected = boot_meta_data->reset_reason & BITMASK_WARM_BOOT;

    // TODO: The disable irq is not defined, need to add a definition
    // __disable_irq(); /* Since we are relocating the vector table */

    // TODO: Send a post code indicating IRQ disabled in SCP/MCP
    // ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484373

    if (is_warm_boot_detected)
    {
        // TODO: Send post code for SCP/MCP warm boot here
        // ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484373
    }
    else
    {
        // TODO: Send post code for SCP/MCP cold boot here
        // ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484373
    }

    SET_LEAVE_IF(!unpack_image(boot_config->data_src_base,
                               image_size,
                               boot_config->itc_ram_base,
                               boot_config->itc_ram_size,
                               boot_config->dtc_ram_base,
                               boot_config->dtc_ram_size),
                 (boot_config->cpu_type == MSCP_CPU_SCP) ? KINGSGATE_BOOT_POSTCODE_E_BOOT_CONFIG_SCP
                                                         : KINGSGATE_BOOT_POSTCODE_E_BOOT_CONFIG_MCP,
                 load_image_failed);

    return (void*)boot_config->itc_ram_base;

load_image_failed:
    // TODO: Send a post code here indicating boot failed
    // ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484373
    if (boot_status != POST_CODE_SCP_OK)
    {
    }

    return NULL;
}