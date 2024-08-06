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
#include <boot_status_codes.h> // for _boot_status_code_t, boot_status_code_t
#include <stdbool.h>           // for false, bool, true
#include <stddef.h>            // for NULL
#include <stdint.h>            // for uint32_t
#include <system_info.h>
#include <unpack_image.h> // for unpack_image

/*-- Symbolic Constant Macros (defines) --*/
// This is based on estimate CORTEX M7 runs @ 1GHz
#define CORTEX_M7_ONE_MS_CPU_CYCLES (1000 * 1000)
#define MIN_SLEEP_MS_RETRY          (2)
// Adjusting it for 1s retry count
#define MAX_RETRY_HSP_MBOX (500)

/*-------------- Typedefs ----------------*/
typedef enum _HSP_MBOX_STATUS_EX
{
    HSP_MBOX_STATUS_NOT_FATAL = 0,
    HSP_MBOX_STATUS_FATAL,
    HSP_MBOX_STATUS_COMPLETE,
} HSP_MBOX_STATUS_EX;

/*--------- Function Prototypes ----------*/
extern void __disable_irq(void);

/*-- Declarations (Statics and globals) --*/
static FPFW_MBX_PRIMITIVE_CTX g_mail_box_context;

/*------------- Functions ----------------*/

#ifndef MBOX_STUB
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
#endif
bool send_post_code(boot_status_code_t boot_post_code, bool is_scp, bool is_fatal)
{
    // TODO: Replace the  mail box data and response with proper structure once it has been defined on HSP
    // side ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1793271
    struct kng_hsp_mailbox_boot_status_notify hsp_mbox_data = {
        .header.cmd = HSP_MAILBOX_CMD_BOOT_STATUS_NOTIFY,
        .header.seq = 0,
        .header.context = 0,
        .header.flags = 0,
        .id = is_scp ? HspFirmwareIdScp : HspFirmwareIdMcp,
        .boot_status = boot_post_code,
        .boot_status_ex = is_fatal ? HSP_MBOX_STATUS_FATAL : HSP_MBOX_STATUS_NOT_FATAL};
    kng_hsp_mailbox_msg hsp_mbox_rsp;

    FPFW_MBX_PAYLOAD mail_box_send_payload = {.payloadBuffer = &hsp_mbox_data,
                                              .payloadSize = (HSP_MBX_FIFO_DEPTH * sizeof(uint32_t))};
    FPFW_MBX_PAYLOAD mail_box_receive_payload = {.payloadBuffer = &hsp_mbox_rsp,
                                                 .payloadSize = (HSP_MBX_FIFO_DEPTH * sizeof(uint32_t))};
    uint32_t count = 0;
    bool ret_code = true;

    if (!system_info_is_hsp_present())
    {
        // If HSP is not present simply return true so boot loader can proceed
        return ret_code;
    }
    if (is_scp && (boot_post_code >= BOOT_STATUS_CODE_SCP_MAX))
    {
        // Post code is out of range of SCP
        return false;
    }

    if (!is_scp && ((boot_post_code < BOOT_STATUS_CODE_MCP_OK) || (boot_post_code >= BOOT_STATUS_CODE_MCP_MAX)))
    {
        // Post code is out of range of MCP
        return false;
    }
    // TODO ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1793271
    // HSP mailbox integration is still not done, hence these apis from SCP/MCP are
    // stubbed out. Stub will be disabled once HSP enables mailbox
#ifdef MBOX_STUB
    // Stubs to make compiler happy
    (void)mail_box_send_payload;
    (void)mail_box_receive_payload;
#else
    // Wait until the mailbox is able to transmit the post code.  At this stage in the boot process this is acceptable.
    while ((count < MAX_RETRY_HSP_MBOX) && (FpFwMailboxSend(&g_mail_box_context, &mail_box_send_payload) != FPFW_MBX_SUCCESS))
    {
        // mailbox send may fail if response is in fifo (w/ additional response also queued); attempt to read/clear any pending response.
        FpFwMailboxReceive(&g_mail_box_context, &mail_box_receive_payload);

        sleep_ms(MIN_SLEEP_MS_RETRY);

        count++;
    }
#endif

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
    uint32_t boot_status = BOOT_STATUS_CODE_SCP_OK;
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

    if (boot_config->cpu_type == MSCP_CPU_SCP)
    {
        is_scp = true;

        boot_status = BOOT_STATUS_CODE_SCP_START;

        mail_box_address = SCP_TOP_SCP2HSP_MAILBOX_ADDRESS;
    }
    else if (boot_config->cpu_type == MSCP_CPU_MCP)
    {
        is_scp = false;

        boot_status = BOOT_STATUS_CODE_MCP_START;

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

    if (send_post_code(boot_status, is_scp, false) == false)
    {
        goto hsp_send_failed;
    }

    if (boot_config->data_src_base == 0)
    {
        boot_status = is_scp ? BOOT_STATUS_CODE_SCP_CONFIG_ERROR : BOOT_STATUS_CODE_MCP_CONFIG_ERROR;
    }
    else if (boot_config->data_src_end <= boot_config->data_src_base)
    {
        boot_status = is_scp ? BOOT_STATUS_CODE_SCP_CONFIG_ERROR : BOOT_STATUS_CODE_MCP_CONFIG_ERROR;
    }
    else if (boot_config->itc_ram_base == 0)
    {
        boot_status = is_scp ? BOOT_STATUS_CODE_SCP_CONFIG_ERROR : BOOT_STATUS_CODE_MCP_CONFIG_ERROR;
    }
    else if (boot_config->itc_ram_size == 0)
    {
        boot_status = is_scp ? BOOT_STATUS_CODE_SCP_CONFIG_ERROR : BOOT_STATUS_CODE_MCP_CONFIG_ERROR;
    }
    else if (boot_config->dtc_ram_base == 0)
    {
        boot_status = is_scp ? BOOT_STATUS_CODE_SCP_CONFIG_ERROR : BOOT_STATUS_CODE_MCP_CONFIG_ERROR;
    }
    else if (boot_config->dtc_ram_size == 0)
    {
        boot_status = is_scp ? BOOT_STATUS_CODE_SCP_CONFIG_ERROR : BOOT_STATUS_CODE_MCP_CONFIG_ERROR;
    }
    else if (boot_config->boot_meta_base == 0)
    {
        boot_status = is_scp ? BOOT_STATUS_CODE_SCP_CONFIG_ERROR : BOOT_STATUS_CODE_MCP_CONFIG_ERROR;
    }
    else
    {
        is_error_config = false;
    }

    if (is_error_config)
    {
        goto load_image_failed;
    }

    __disable_irq();

    boot_status = is_scp ? BOOT_STATUS_CODE_SCP_IRQ_DISABLED : BOOT_STATUS_CODE_MCP_IRQ_DISABLED;

    if (send_post_code(boot_status, is_scp, false) == false)
    {
        goto hsp_send_failed;
    }

    if (boot_meta_data->ResetReason & BITMASK_WARM_BOOT)
    {
        boot_status = is_scp ? BOOT_STATUS_CODE_SCP_WARM_BOOT : BOOT_STATUS_CODE_MCP_WARM_BOOT;
    }
    else
    {
        boot_status = is_scp ? BOOT_STATUS_CODE_SCP_COLD_BOOT : BOOT_STATUS_CODE_MCP_COLD_BOOT;
    }

    if (send_post_code(boot_status, is_scp, false) == false)
    {
        goto hsp_send_failed;
    }

    image_size = (uint32_t)(boot_config->data_src_end - boot_config->data_src_base) + 1;

    if (unpack_image(boot_config->data_src_base,
                     image_size,
                     boot_config->itc_ram_base,
                     boot_config->itc_ram_size,
                     boot_config->dtc_ram_base,
                     boot_config->dtc_ram_size) == false)
    {
        // Unpack image mainly fails due to size mismatch or decompress failure. However no status code
        // for decompress failure.
        boot_status = is_scp ? BOOT_STATUS_CODE_SCP_CONFIG_ERROR : BOOT_STATUS_CODE_MCP_CONFIG_ERROR;
        goto load_image_failed;
    }

    return (void*)boot_config->itc_ram_base;

load_image_failed:
    if (send_post_code(boot_status, is_scp, true) == true)
    {
        return NULL;
    }

hsp_send_failed:
    // There is a boot status for this , but there is no way to send this out
    boot_status = is_scp ? BOOT_STATUS_CODE_SCP_CONFIG_ERROR : BOOT_STATUS_CODE_MCP_CONFIG_ERROR;
    (void)boot_status;
    return NULL;
}
