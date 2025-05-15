//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file kingsgate_boot.c
 * This file contains the load_image function for the MCP/SCP boot loader. It invokes unpack_image
 * API to decompress the MCP/SCP FW to the TCMs and return ITCM base address on success and NULL
 * on failure.
 */
#ifndef UNIT_TEST
    #include <cmsis_compiler.h>
#endif
#include <MboxPrimitives.h>       // for FPFW_MBX_FIFO_DEPTH, FPFW_MBX_STATUS
#include <hsp_firmware_headers.h> // for HSP_MAILBOX_CMD_BOOT_STATUS_NOTIFY, HspFirmwareIdScp...
#include <hsp_interaction_i.h>    // for sleep_ms, send_post_code, hsp_...
#include <kingsgate_boot.h>       // for kingsgate_boot_metadata_t, BITMASK_W...
#ifdef UNIT_TEST
    #include <mock_scp_mcp_top.h> // for MCP_TOP_MCP2HSP_MAILBOX_ADDRESS, SCP...
#else
    #include <mcp_top_regs.h>
    #include <scp_top_regs.h>
#endif
#include <boot_status.h> // for _mscp_boot_status_code_t, mscp_boot_status_code_t
#include <boot_status_codes.h>
#include <stdbool.h> // for false, bool, true
#include <stddef.h>  // for NULL
#include <stdint.h>  // for uint32_t
#include <system_info.h>
#include <unpack_image.h> // for unpack_image

/*-- Symbolic Constant Macros (defines) --*/
// This is based on estimate CORTEX M7 runs @ 1GHz
#define CORTEX_M7_ONE_MS_CPU_CYCLES (1000 * 1000)
#define MIN_SLEEP_MS_RETRY          (2)
// Adjusting it for 1s retry count
#define MAX_RETRY_HSP_MBOX (500)

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
extern void __disable_irq(void);

/*-- Declarations (Statics and globals) --*/
static FPFW_MBX_PRIMITIVE_CTX g_mail_box_context;
static struct kng_hsp_mailbox_boot_status_extd_notify g_hsp_mbox_data = {
    .header =
        {
            .cmd = HSP_MAILBOX_CMD_BOOT_STATUS_EXTD_NOTIFY,
            .seq = 0,
            .context = 0,
            .flags = 0,
        },
    .id = HSP_FIRMWARE_ID_SCP,
    .boot_status = MSCP_BOOT_STATUS_CODE_SCP_OK,
    .boot_status_ex =
        {
            .component_group = COMPONENT_GROUP_SCP,
            .component_subgroup = MSCP_BOOTLOADER,
            .component_instance = SCP_PRIMARY,
            .reserved = 0x0,
        },
};

/*------------- Functions ----------------*/
void sleep_ms(uint32_t millisecond)
{
    uint32_t count = 0;
    uint32_t i = 0;
    while (count < millisecond)
    {
        for (i = 0; i < CORTEX_M7_ONE_MS_CPU_CYCLES; i++)
        {
            asm volatile("nop");
        }

        count++;
    }
}

bool send_post_code(void* boot_status_msg)
{
    uint32_t count = 0;
    bool ret_code = true;

    if (!system_info_is_hsp_present())
    {
        // If HSP is not present simply return true so boot loader can proceed
        return ret_code;
    }

    if (boot_status_msg == NULL)
    {
        return false;
    }
    struct kng_hsp_mailbox_boot_status_extd_notify* msg = (struct kng_hsp_mailbox_boot_status_extd_notify*)boot_status_msg;

    //! Verify range of the boot status code
    if ((msg->id == HSP_FIRMWARE_ID_MCP) &&
        ((msg->boot_status < MSCP_BOOT_STATUS_CODE_MCP_OK) || (msg->boot_status >= MSCP_BOOT_STATUS_CODE_MCP_MAX)))
    {
        // Post code is out of range of MCP
        return false;
    }
    if ((msg->id == HSP_FIRMWARE_ID_SCP) &&
        ((msg->boot_status < MSCP_BOOT_STATUS_CODE_SCP_OK) || (msg->boot_status >= MSCP_BOOT_STATUS_CODE_SCP_MAX)))
    {
        // Post code is out of range of SCP
        return false;
    }

    FPFW_MBX_PAYLOAD mail_box_send_payload = {.payloadBuffer = msg,
                                              .payloadSize = (HSP_MBX_FIFO_DEPTH * sizeof(uint32_t))};

    //! Wait until the mailbox is able to transmit the post code.  At this stage in the boot process this is acceptable.
    while ((count < MAX_RETRY_HSP_MBOX) && (FpFwMailboxSend(&g_mail_box_context, &mail_box_send_payload) != FPFW_MBX_SUCCESS))
    {
        sleep_ms(MIN_SLEEP_MS_RETRY);
        count++;
    }
    if (count >= MAX_RETRY_HSP_MBOX)
    {
        ret_code = false;
    }
    return ret_code;
}

bool hsp_mailbox_init(uint32_t mail_box_address)
{
    FPFW_MBX_REG_CONFIG mail_box_config = {.MbxFifoDepth = HSP_MBX_FIFO_DEPTH,
                                           .MbxMesgHandlingType = MBX_MESG_HANDLING_SINGLE_MESG_AT_A_TIME,
                                           .MbxImplementation = MBX_IMPL_POLLING,
                                           .MsgSizeBytes = (HSP_MBX_FIFO_DEPTH * sizeof(uint32_t)),
                                           .MbxBaseAddr = mail_box_address};

    if (mail_box_address == 0)
    {
        return false;
    }

    if (FpFwMailboxInit(&mail_box_config, &g_mail_box_context) != FPFW_MBX_SUCCESS)
    {
        return false;
    }

    if (FpFwMailboxFlushFIFO(&g_mail_box_context) != FPFW_MBX_SUCCESS)
    {
        return false;
    }

    return true;
}

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
    uint32_t mail_box_address = 0;
    uint32_t boot_status = MSCP_BOOT_STATUS_CODE_SCP_OK;
    uint32_t image_size = 0;
    bool is_scp = false;
    bool is_error_config = true;

    HSP_BOOT_METADATA* boot_meta_data = NULL;

    // If boot config is null or cpu type is invalid we simply return as there is nothing we can do
    // dont know where to send status code
    if (boot_config == NULL)
    {
        return NULL;
    }

    // Boot metadata is expected to be stored at the beginning of the MSCP_EXP SRAM chosen
    boot_meta_data = (HSP_BOOT_METADATA*)(boot_config->boot_meta_base);
    if (!boot_meta_data)
    {
        // Boot metadata is not available, can't proceed, return NULL
        return NULL;
    }

    uint8_t current_die = boot_meta_data->CurrentDie;
    if (boot_config->cpu_type == MSCP_CPU_SCP)
    {
        is_scp = true;
        g_hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
        g_hsp_mbox_data.boot_status = MSCP_BOOT_STATUS_CODE_SCP_OK;
        g_hsp_mbox_data.boot_status_ex.component_group = COMPONENT_GROUP_SCP;
        g_hsp_mbox_data.boot_status_ex.component_subgroup = MSCP_BOOTLOADER;
        g_hsp_mbox_data.boot_status_ex.component_instance = (current_die == 0) ? SCP_PRIMARY : SCP_SECONDARY;
        g_hsp_mbox_data.boot_status_ex.reserved = (current_die == 0) ? BOOT_STATUS_CODE_SCP0_OK : BOOT_STATUS_CODE_SCP1_OK;
        mail_box_address = SCP_TOP_SCP2HSP_MAILBOX_ADDRESS;
    }
    else if (boot_config->cpu_type == MSCP_CPU_MCP)
    {
        is_scp = false;
        g_hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
        g_hsp_mbox_data.boot_status = MSCP_BOOT_STATUS_CODE_MCP_OK;
        g_hsp_mbox_data.boot_status_ex.component_group = COMPONENT_GROUP_MCP;
        g_hsp_mbox_data.boot_status_ex.component_subgroup = MSCP_BOOTLOADER;
        g_hsp_mbox_data.boot_status_ex.component_instance = (current_die == 0) ? MCP_PRIMARY : MCP_SECONDARY;
        g_hsp_mbox_data.boot_status_ex.reserved = (current_die == 0) ? BOOT_STATUS_CODE_MCP0_OK : BOOT_STATUS_CODE_MCP1_OK;
        mail_box_address = MCP_TOP_MCP2HSP_MAILBOX_ADDRESS;
    }
    else
    {
        // This is highly unlikely but added from logic perspective.
        return NULL;
    }

    // Init the mail box here
    if (hsp_mailbox_init(mail_box_address) == false)
    {
        return NULL;
    }
    //! Init system info here
    system_info_init(NULL);
    //! Send the very 1st boot status to HSP
    if (send_post_code(&g_hsp_mbox_data) == false)
    {
        goto hsp_send_failed;
    }
    //! Reset hsp led code post sending the 1st boot status
    g_hsp_mbox_data.boot_status_ex.reserved = 0x0;

    //! Validate the boot config
    if (boot_config->data_src_base == 0)
    {
        boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG : MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    }
    else if (boot_config->data_src_end <= boot_config->data_src_base)
    {
        boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG : MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    }
    else if (boot_config->itc_ram_base == 0)
    {
        boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG : MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    }
    else if (boot_config->itc_ram_size == 0)
    {
        boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG : MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    }
    else if (boot_config->dtc_ram_base == 0)
    {
        boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG : MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    }
    else if (boot_config->dtc_ram_size == 0)
    {
        boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG : MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    }
    else if (boot_config->boot_meta_base == 0)
    {
        boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG : MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    }
    else if (boot_config->rmss_data_base == 0)
    {
        boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG : MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    }
    else if (boot_config->rmss_data_size == 0)
    {
        boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG : MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    }
    else if ((boot_meta_data->ResetReason & BITMASK_WARM_BOOT) && (boot_meta_data->BootMode != 0))
    {
        boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG : MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    }
    else
    {
        is_error_config = false;
    }

    if (is_error_config)
    {
        g_hsp_mbox_data.boot_status = boot_status;
        goto load_image_failed;
    }

    __disable_irq();

    g_hsp_mbox_data.boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_IRQ_DISABLED : MSCP_BOOT_STATUS_CODE_MCP_IRQ_DISABLED;

    if (send_post_code(&g_hsp_mbox_data) == false)
    {
        goto hsp_send_failed;
    }

    if (boot_meta_data->ResetReason & BITMASK_WARM_BOOT)
    {
        g_hsp_mbox_data.boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_WARM_BOOT : MSCP_BOOT_STATUS_CODE_MCP_WARM_BOOT;
    }
    else
    {
        g_hsp_mbox_data.boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_COLD_BOOT : MSCP_BOOT_STATUS_CODE_MCP_COLD_BOOT;
    }

    if (send_post_code(&g_hsp_mbox_data) == false)
    {
        goto hsp_send_failed;
    }

    image_size = (uint32_t)(boot_config->data_src_end - boot_config->data_src_base) + 1;

    if (unpack_image(boot_config->data_src_base,
                     image_size,
                     boot_config->itc_ram_base,
                     boot_config->itc_ram_size,
                     boot_config->dtc_ram_base,
                     boot_config->dtc_ram_size,
                     boot_config->rmss_data_base,
                     boot_config->rmss_data_size) == false)
    {
        // Unpack image mainly fails due to size mismatch or decompress failure. However no status code
        // for decompress failure.
        g_hsp_mbox_data.boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG : MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
        goto load_image_failed;
    }

    return (void*)boot_config->itc_ram_base;

load_image_failed:
    if (send_post_code(&g_hsp_mbox_data) == true)
    {
        return NULL;
    }

hsp_send_failed:
    // There is a boot status for this , but there is no way to send this out
    g_hsp_mbox_data.boot_status = is_scp ? MSCP_BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG : MSCP_BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    (void)g_hsp_mbox_data.boot_status;
    return NULL;
}
