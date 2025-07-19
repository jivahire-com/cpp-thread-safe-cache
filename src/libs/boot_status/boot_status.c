//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file boot_status.c
 * Implements Boot Status APIs
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwLock.h> // for FPFW_LOCK, FpFwLockInitialize, FpFwLockAcquire, FpFwLockRelease
#include <boot_status.h>
#include <boot_status_codes.h>
#include <bug_check.h>
#include <fpfw_icc_base.h>                  // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <hsp_firmware_headers.h>           // for HSP_FIRMWARE_ID
#include <idsw.h>                           // for idsw_get_cpu_type
#include <idsw_kng.h>                       // for KNG_CPU_TYPE
#include <inttypes.h>                       // for PRId32, PRIx32
#include <kingsgate_hsp_mailbox_commands.h> // for kng_hsp_mailbox_msg
#include <stdint.h>
#include <stdio.h>       // for printf
#include <string.h>      // for memcpy
#include <system_info.h> // for system_info_is_init_complete

/*------------- Typedefs -----------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static boot_status_icc_ctx_t* boot_status_icc_ctx = NULL; //! store boot status ctx from init

/*------------- Functions ----------------*/
void boot_status_init(boot_status_icc_ctx_t* boot_status_ctx)
{
    BUG_ASSERT(boot_status_ctx != NULL);              //! check if the boot status ctx is valid
    BUG_ASSERT(boot_status_ctx->hsp_mbx_ctx != NULL); //! check if the hsp mbox ctx is valid
    boot_status_icc_ctx = boot_status_ctx;            //! store the boot status ctx from init
}

void boot_status_reset(void)
{
    boot_status_icc_ctx = NULL; //! store boot status ctx from init
    FPFW_DBGPRINT_INFO("[Boot Status] Reset completed.\n");
}

static void boot_status_extd_notify_complete(void* context, fpfw_status_t status)
{
    boot_status_req_t* req = (boot_status_req_t*)context;
    BUG_ASSERT(status == FPFW_STATUS_SUCCESS); //! status of the icc base communication
    BUG_ASSERT(req != NULL);
    BUG_ASSERT(req->msg.header.cmd == HSP_MAILBOX_CMD_BOOT_STATUS_EXTD_NOTIFY); //! msg is a boot status notify message

    //! call the user provided cb is provided
    if (req->cb != NULL)
    {
        req->cb(req->cb_ctx);
    }

    //! decode the boot status extended field
    uint8_t decoded_group = 0;
    uint8_t decoded_subgroup = 0;
    uint8_t decoded_instance = 0;
    uint8_t decoded_status = 0;
    DECODE_BOOT_STATUS_EX_LED_CODE(req->msg.boot_stat_extd_notif.boot_status_ex.boot_status_int,
                                   decoded_group,
                                   decoded_subgroup,
                                   decoded_instance,
                                   decoded_status);

    //! Print the boot status notification message on uart
    FPFW_DBGPRINT_INFO("[Boot Status Extd] Async Notif Send Complete, Cmd[0x%" PRIx16 "] ID[%" PRId32
                       "] Stat[0x%" PRIx32 "] Stat_ex[0x%" PRIx32 "] : Grp[0x%02" PRIx8 "] Subgrp[0x%02" PRIx8
                       "] Inst[0x%02" PRIx8 "] Stat[0x%02" PRIx8 "]\n",
                       req->msg.header.cmd,
                       req->msg.boot_stat_extd_notif.id,
                       req->msg.boot_stat_extd_notif.boot_status,
                       req->msg.boot_stat_extd_notif.boot_status_ex.boot_status_int,
                       decoded_group,
                       decoded_subgroup,
                       decoded_instance,
                       decoded_status);
}

static bool check_boot_status_ex_param_validity(uint32_t boot_status_ex, KNG_CPU_TYPE cpu_type, KNG_DIE_ID die_id)
{
    bool is_valid_led_status = false;

    //! 1st Check for valid input range
    //! SCP, MCP, SDM & CDED allowed
    uint8_t component_group = (boot_status_ex & 0xFF);
    if ((component_group != COMPONENT_GROUP_SCP) && (component_group != COMPONENT_GROUP_MCP) &&
        (component_group != COMPONENT_GROUP_SDM) && (component_group != COMPONENT_GROUP_CDED))
    {
        FPFW_DBGPRINT_ERROR("[Boot Status] Invalid component group[0x%" PRIx32 "]\n", component_group);
        return false;
    }
    //! Valid range is MSCP_GENERIC to MSCP_SUBGROUP_MAX
    uint8_t sub_component_group = ((boot_status_ex >> 8) & 0xFF);
    if (sub_component_group >= MSCP_SUBGROUP_MAX)
    {
        FPFW_DBGPRINT_ERROR("[Boot Status] Invalid sub component group[0x%" PRIx32 "]\n", sub_component_group);
        return false;
    }
    //! Valid range is SCP_PRIMARY to MAX_COMPONENT_INSTANCE
    uint8_t instance = ((boot_status_ex >> 16) & 0xFF);
    if (instance >= MAX_COMPONENT_INSTANCE)
    {
        FPFW_DBGPRINT_ERROR("[Boot Status] Invalid instance[0x%" PRIx32 "]\n", instance);
        return false;
    }
    //! 8 bit reserved led status can be either 0 or 0x40 to 0x7f
    uint8_t led_status = ((boot_status_ex >> 24) & 0xFF);
    if (!((led_status == 0) || ((led_status >= BOOT_STATUS_CODE_SCP0_OK) && (led_status <= BOOT_STATUS_CODE_CDED1_HW_FAULT))))
    {
        FPFW_DBGPRINT_ERROR("[Boot Status] Invalid led status[0x%" PRIx32 "]\n", led_status);
        return false;
    }

    //! 2nd Check for valid component group & instance based on current cpu type & die id
    if (cpu_type == CPU_SCP)
    {
        if ((component_group != COMPONENT_GROUP_SCP) && (component_group != COMPONENT_GROUP_SDM) &&
            (component_group != COMPONENT_GROUP_CDED))
        {
            FPFW_DBGPRINT_ERROR("[Boot Status] Invalid component group[0x%" PRIx32
                                "] for current cpu type[0x%" PRIx32 "]\n",
                                component_group,
                                cpu_type);
            return false;
        }

        if (die_id == DIE_0)
        {
            if (((component_group == COMPONENT_GROUP_SCP) && (instance != SCP_PRIMARY)) ||
                ((component_group == COMPONENT_GROUP_SDM) && (instance != SDM_PRIMARY)) ||
                ((component_group == COMPONENT_GROUP_CDED) && (instance != CDED_PRIMARY)))
            {
                FPFW_DBGPRINT_ERROR("[Boot Status] Invalid instance[0x%" PRIx32
                                    "] for component group[0x%" PRIx32 "]\n",
                                    instance,
                                    component_group);
                return false;
            }
        }
        else // DIE 1
        {
            if (((component_group == COMPONENT_GROUP_SCP) && (instance != SCP_SECONDARY)) ||
                ((component_group == COMPONENT_GROUP_SDM) && (instance != SDM_SECONDARY)) ||
                ((component_group == COMPONENT_GROUP_CDED) && (instance != CDED_SECONDARY)))
            {
                FPFW_DBGPRINT_ERROR("[Boot Status] Invalid instance[0x%" PRIx32
                                    "] for component group[0x%" PRIx32 "]\n",
                                    instance,
                                    component_group);
                return false;
            }
        }
    }
    else // MCP
    {
        if (component_group != COMPONENT_GROUP_MCP)
        {
            FPFW_DBGPRINT_ERROR("[Boot Status] Invalid component group[0x%" PRIx32
                                "] for current cpu type[0x%" PRIx32 "]\n",
                                component_group,
                                cpu_type);
            return false;
        }

        if ((die_id == DIE_0 && instance != MCP_PRIMARY) || (die_id == DIE_1 && instance != MCP_SECONDARY))
        {
            FPFW_DBGPRINT_ERROR("[Boot Status] Invalid instance[0x%" PRIx32 "] for component group[0x%" PRIx32 "]\n",
                                instance,
                                component_group);
            return false;
        }
    }

    //! 3rd, Now that range, component group & instance are validated, check for led status combination
    if (led_status == 0)
    {
        return true; //! led status is 0, so valid
    }

    switch (instance)
    {
    case SCP_PRIMARY:
        is_valid_led_status =
            (led_status == BOOT_STATUS_CODE_SCP_ACCEL_FAILED) ||
            (((led_status >= BOOT_STATUS_CODE_SCP0_OK) && (led_status <= BOOT_STATUS_CODE_SCP0_BOOT_COMPLETE)) ||
             ((led_status >= BOOT_STATUS_CODE_SCP_E_DDR0_TRAINING) && (led_status <= BOOT_STATUS_CODE_SCP_E_DDR5_TRAINING)));
        break;
    case SCP_SECONDARY:
        is_valid_led_status =
            (led_status == BOOT_STATUS_CODE_SCP_ACCEL_FAILED) ||
            (((led_status >= BOOT_STATUS_CODE_SCP1_OK) && (led_status <= BOOT_STATUS_CODE_SCP1_BOOT_COMPLETE)) ||
             ((led_status >= BOOT_STATUS_CODE_SCP1_E_DDR6_TRAINING) && (led_status <= BOOT_STATUS_CODE_SCP1_E_DDR11_TRAINING)));
        break;
    case MCP_PRIMARY:
        is_valid_led_status =
            (led_status == BOOT_STATUS_CODE_MCP0_OK) || (led_status == BOOT_STATUS_CODE_MCP0_BOOT_COMPLETE);
        break;
    case MCP_SECONDARY:
        is_valid_led_status =
            (led_status == BOOT_STATUS_CODE_MCP1_OK) || (led_status == BOOT_STATUS_CODE_MCP1_BOOT_COMPLETE);
        break;
    case SDM_PRIMARY:
        is_valid_led_status = (led_status == BOOT_STATUS_CODE_SDM0_OK) ||
                              (led_status == BOOT_STATUS_CODE_SDM0_BOOT_COMPLETE) ||
                              (led_status == BOOT_STATUS_CODE_SDM0_HW_FAULT);
        break;
    case SDM_SECONDARY:
        is_valid_led_status = (led_status == BOOT_STATUS_CODE_SDM1_OK) ||
                              (led_status == BOOT_STATUS_CODE_SDM1_BOOT_COMPLETE) ||
                              (led_status == BOOT_STATUS_CODE_SDM1_HW_FAULT);
        break;
    case CDED_PRIMARY:
        is_valid_led_status = (led_status == BOOT_STATUS_CODE_CDED0_OK) ||
                              (led_status == BOOT_STATUS_CODE_CDED0_BOOT_COMPLETE) ||
                              (led_status == BOOT_STATUS_CODE_CDED0_HW_FAULT);
        break;
    case CDED_SECONDARY:
        is_valid_led_status = (led_status == BOOT_STATUS_CODE_CDED1_OK) ||
                              (led_status == BOOT_STATUS_CODE_CDED1_BOOT_COMPLETE) ||
                              (led_status == BOOT_STATUS_CODE_CDED1_HW_FAULT);
        break;
    default:
        is_valid_led_status = false; //! invalid instance, so invalid led status
        break;
    }

    if (!is_valid_led_status)
    {
        FPFW_DBGPRINT_ERROR("[Boot Status] Invalid led status[0x%" PRIx32 "] for instance[0x%" PRIx32 "]\n", led_status, instance);
        return false;
    }
    return true;
}

static uint8_t convert_led_status_to_boot_status(uint8_t led_status)
{
    uint8_t boot_status = 0xFF; //! invalid boot status
    KNG_DIE_ID die_id = idsw_get_die_id();
    KNG_CPU_TYPE cpu_type = idsw_get_cpu_type(); //! get the cpu type, scp/mcp

    if ((cpu_type == CPU_MCP) &&
        ((led_status != LED_STATUS_CODE_MCP_OK) && (led_status != LED_STATUS_CODE_MCP_BOOT_COMPLETE)))
    {
        FPFW_DBGPRINT_ERROR("[Post LED Status] Invalid status code[0x%" PRIx32 "]\n", led_status);
        BUG_ASSERT(false); //! should not reach here
    }

    switch (led_status)
    {
    case LED_STATUS_CODE_SCP_ACCEL_FAILED:
        boot_status = BOOT_STATUS_CODE_SCP_ACCEL_FAILED;
        break;
    case LED_STATUS_CODE_SCP_OK:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SCP0_OK : BOOT_STATUS_CODE_SCP1_OK;
        break;
    case LED_STATUS_CODE_MCP_OK:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_MCP0_OK : BOOT_STATUS_CODE_MCP1_OK;
        break;
    case LED_STATUS_CODE_SDM_OK:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SDM0_OK : BOOT_STATUS_CODE_SDM1_OK;
        break;
    case LED_STATUS_CODE_CDED_OK:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_CDED0_OK : BOOT_STATUS_CODE_CDED1_OK;
        break;
    case LED_STATUS_CODE_SCP_BOOT_COMPLETE:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SCP0_BOOT_COMPLETE : BOOT_STATUS_CODE_SCP1_BOOT_COMPLETE;
        break;
    case LED_STATUS_CODE_MCP_BOOT_COMPLETE:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_MCP0_BOOT_COMPLETE : BOOT_STATUS_CODE_MCP1_BOOT_COMPLETE;
        break;
    case LED_STATUS_CODE_SDM_BOOT_COMPLETE:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SDM0_BOOT_COMPLETE : BOOT_STATUS_CODE_SDM1_BOOT_COMPLETE;
        break;
    case LED_STATUS_CODE_CDED_BOOT_COMPLETE:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_CDED0_BOOT_COMPLETE : BOOT_STATUS_CODE_CDED1_BOOT_COMPLETE;
        break;
    case LED_STATUS_CODE_SCP_MESH_INIT_START:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SCP0_MESH_INIT_START : BOOT_STATUS_CODE_SCP1_MESH_INIT_START;
        break;
    case LED_STATUS_CODE_SCP_TOWER_INIT_START:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SCP0_TOWER_INIT_START : BOOT_STATUS_CODE_SCP1_TOWER_INIT_START;
        break;
    case LED_STATUS_CODE_SCP_ACCEL_INIT_START:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SCP0_ACCEL_INIT_START : BOOT_STATUS_CODE_SCP1_ACCEL_INIT_START;
        break;
    case LED_STATUS_CODE_SCP_DDR_INIT_START:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SCP0_DDR_INIT_START : BOOT_STATUS_CODE_SCP1_DDR_INIT_START;
        break;
    case LED_STATUS_CODE_SDM_HW_FAULT:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SDM0_HW_FAULT : BOOT_STATUS_CODE_SDM1_HW_FAULT;
        break;
    case LED_STATUS_CODE_CDED_HW_FAULT:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_CDED0_HW_FAULT : BOOT_STATUS_CODE_CDED1_HW_FAULT;
        break;
    case LED_STATUS_CODE_SCP_E_DDR0_TRAINING:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SCP_E_DDR0_TRAINING : BOOT_STATUS_CODE_SCP1_E_DDR6_TRAINING;
        break;
    case LED_STATUS_CODE_SCP_E_DDR1_TRAINING:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SCP_E_DDR1_TRAINING : BOOT_STATUS_CODE_SCP1_E_DDR7_TRAINING;
        break;
    case LED_STATUS_CODE_SCP_E_DDR2_TRAINING:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SCP_E_DDR2_TRAINING : BOOT_STATUS_CODE_SCP1_E_DDR8_TRAINING;
        break;
    case LED_STATUS_CODE_SCP_E_DDR3_TRAINING:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SCP_E_DDR3_TRAINING : BOOT_STATUS_CODE_SCP1_E_DDR9_TRAINING;
        break;
    case LED_STATUS_CODE_SCP_E_DDR4_TRAINING:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SCP_E_DDR4_TRAINING : BOOT_STATUS_CODE_SCP1_E_DDR10_TRAINING;
        break;
    case LED_STATUS_CODE_SCP_E_DDR5_TRAINING:
        boot_status = (die_id == DIE_0) ? BOOT_STATUS_CODE_SCP_E_DDR5_TRAINING : BOOT_STATUS_CODE_SCP1_E_DDR11_TRAINING;
        break;
    default:
        break;
    }
    return boot_status;
}

void boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    KNG_CPU_TYPE cpu_type = idsw_get_cpu_type(); //! get the cpu type, scp/mcp
    KNG_DIE_ID die_id = idsw_get_die_id();       //! get the die id
    fpfw_status_t status = FPFW_STATUS_SUCCESS;
    bool is_icc_sync = false; //! cache the request type in case `is_runtime` flag updates

    //! Null check for the request memory
    BUG_ASSERT(p_req_mem != NULL);
    //! check validity of boot_status_ex bit fields
    BUG_ASSERT(check_boot_status_ex_param_validity(boot_status_ex, cpu_type, die_id));
    //! check if hsp is present
    BUG_ASSERT(system_info_is_hsp_present());
    //! check if icc ctx is set
    BUG_ASSERT(boot_status_icc_ctx != NULL);
    BUG_ASSERT(boot_status_icc_ctx->hsp_mbx_ctx != NULL);

    //! Prepare the hsp message packet
    p_req_mem->msg.boot_stat_extd_notif.header.cmd = HSP_MAILBOX_CMD_BOOT_STATUS_EXTD_NOTIFY;
    p_req_mem->msg.boot_stat_extd_notif.id = (cpu_type == CPU_SCP) ? HSP_FIRMWARE_ID_SCP : HSP_FIRMWARE_ID_MCP;
    p_req_mem->msg.boot_stat_extd_notif.boot_status = boot_status;
    p_req_mem->msg.boot_stat_extd_notif.boot_status_ex.boot_status_int = boot_status_ex;

    //! Check current state of the system, init or runtime
    if (system_info_is_init_complete())
    {
        //! Prepare async send request
        p_req_mem->hsp_send_params.payload_buffer = &p_req_mem->msg;
        p_req_mem->hsp_send_params.buffer_size = sizeof(p_req_mem->msg);
        p_req_mem->hsp_send_params.cb = boot_status_extd_notify_complete;
        p_req_mem->hsp_send_params.cb_ctx = p_req_mem;

        //! Raise an async hsp mbox send request to notify hsp of boot status
        //! Hsp will not send a response for this message, so no need to wait for a response
        status = fpfw_icc_base_send(boot_status_icc_ctx->hsp_mbx_ctx, &p_req_mem->hsp_send_params);
    }
    else
    {
        //! Raise sync hsp mbox request to notify hsp of boot status, blocking call, will wait until send is
        //! complete, ie data pushed to fifo or timeout occurs
        status = fpfw_icc_base_send_sync(boot_status_icc_ctx->hsp_mbx_ctx, &p_req_mem->msg, sizeof(p_req_mem->msg));
        is_icc_sync = true;
    }

    if (status != FPFW_STATUS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("[Boot Status Extd] %s Notif Send Failed, Status[0x%" PRIx32 "]\n",
                            ((is_icc_sync) ? "Sync" : "Async"),
                            status);
        BUG_ASSERT(status == FPFW_STATUS_SUCCESS); //! status of the icc base communication
    }
    else
    {
        //! decode the boot status extended code
        uint8_t decoded_group = 0;
        uint8_t decoded_subgroup = 0;
        uint8_t decoded_instance = 0;
        uint8_t decoded_status = 0;
        DECODE_BOOT_STATUS_EX_LED_CODE(p_req_mem->msg.boot_stat_extd_notif.boot_status_ex.boot_status_int,
                                       decoded_group,
                                       decoded_subgroup,
                                       decoded_instance,
                                       decoded_status);

        FPFW_DBGPRINT_INFO("[Boot Status Extd] %s Notif Send %s, Cmd[0x%" PRIx16 "] ID[%" PRId32
                           "] Stat[0x%" PRIx32 "] Stat_ex[0x%" PRIx32 "]: Grp[0x%02" PRIx8
                           "] Subgrp[0x%02" PRIx8 "] Inst[0x%02" PRIx8 "] Stat[0x%02" PRIx8 "]\n",
                           ((is_icc_sync) ? "Sync" : "Async"),
                           ((is_icc_sync) ? "Completed" : "Raised"),
                           p_req_mem->msg.header.cmd,
                           p_req_mem->msg.boot_stat_extd_notif.id,
                           p_req_mem->msg.boot_stat_extd_notif.boot_status,
                           p_req_mem->msg.boot_stat_extd_notif.boot_status_ex.boot_status_int,
                           decoded_group,
                           decoded_subgroup,
                           decoded_instance,
                           decoded_status);

        //! call the callback function if provided & sync request is raised since the request is completed immediately
        if (is_icc_sync && p_req_mem->cb != NULL)
        {
            p_req_mem->cb(p_req_mem->cb_ctx);
        }
    }
}

void post_led_status(boot_status_req_t* p_req_mem, led_status_codes_t status)
{
    //! Null check for the request memory
    BUG_ASSERT(p_req_mem != NULL);
    BUG_ASSERT(status < LED_STATUS_CODE_MAX); //! check if the status code is valid

    //! Keep the boot_status field as unused, we will populate the boot_status_ex field
    //! of struct kng_hsp_mailbox_boot_status_extd_notify
    mscp_boot_status_code_t boot_status = MSCP_BOOT_STATUS_CODE_UNUSED;
    uint8_t led_status = convert_led_status_to_boot_status(status);
    boot_status_component_subgroup_t sub_group = MSCP_GENERIC; //! Use generic subgroup

    //! Get group from status code
    boot_status_component_group_t group = 0;
    if ((led_status >= BOOT_STATUS_CODE_SCP0_OK && led_status <= BOOT_STATUS_CODE_SCP1_BOOT_COMPLETE) ||
        (led_status >= BOOT_STATUS_CODE_SCP_ACCEL_FAILED && led_status <= BOOT_STATUS_CODE_SCP1_E_DDR11_TRAINING))
    {
        group = COMPONENT_GROUP_SCP;
    }
    else if (led_status >= BOOT_STATUS_CODE_MCP0_OK && led_status <= BOOT_STATUS_CODE_MCP1_BOOT_COMPLETE)
    {
        group = COMPONENT_GROUP_MCP;
    }
    else if ((led_status >= BOOT_STATUS_CODE_SDM0_OK && led_status <= BOOT_STATUS_CODE_SDM1_BOOT_COMPLETE) ||
             (led_status == BOOT_STATUS_CODE_SDM0_HW_FAULT) || (led_status == BOOT_STATUS_CODE_SDM1_HW_FAULT))
    {
        group = COMPONENT_GROUP_SDM;
    }
    else if ((led_status >= BOOT_STATUS_CODE_CDED0_OK && led_status <= BOOT_STATUS_CODE_CDED1_BOOT_COMPLETE) ||
             (led_status == BOOT_STATUS_CODE_CDED0_HW_FAULT) || (led_status == BOOT_STATUS_CODE_CDED1_HW_FAULT))
    {
        group = COMPONENT_GROUP_CDED;
    }
    else
    {
        FPFW_DBGPRINT_ERROR("[Post LED Status] Invalid status code[0x%" PRIx32 "]\n", status);
        BUG_ASSERT(false); //! should not reach here
    }

    //! Get instance from group
    KNG_DIE_ID die_id = idsw_get_die_id(); //! get the die id
    boot_status_component_instance_t instance = 0;
    if (group == COMPONENT_GROUP_SCP)
    {
        instance = (die_id == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY;
    }
    else if (group == COMPONENT_GROUP_MCP)
    {
        instance = (die_id == DIE_0) ? MCP_PRIMARY : MCP_SECONDARY;
    }
    else if (group == COMPONENT_GROUP_SDM)
    {
        instance = (die_id == DIE_0) ? SDM_PRIMARY : SDM_SECONDARY;
    }
    else //! group is COMPONENT_GROUP_CDED
    {
        instance = (die_id == DIE_0) ? CDED_PRIMARY : CDED_SECONDARY;
    }

    //! Generate the extended boot status code
    uint32_t boot_status_ex = GEN_BOOT_STATUS_EX_LED_CODE(group, sub_group, instance, led_status);
    //! call the raw API for boot status notify
    boot_status_notify_extd(p_req_mem, boot_status, boot_status_ex);
}