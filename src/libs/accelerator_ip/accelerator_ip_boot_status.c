//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_boot_status.c
 * Accelerator cores boot status code APIs
 */

/*-------------------------------- Includes ---------------------------------*/
#include "accelerator_ip.h"
#include "accelerator_ip_priv.h"

#include <DbgPrint.h>          // for FPFW_DBGPRINT_INFO, FPFW_DBGPRINT_ERROR
#include <FpFwUtils.h>         // for FPFW_DUR_S
#include <accelip_id.h>        // for NUM_VALID_ACCEL_ID, ACCEL_ID_SDM
#include <boot_status.h>       // for boot_status_notify_extd
#include <boot_status_codes.h> // for BOOT_STATUS_CODE_SDM0_BOOT_COMPLETE, BOOT_STATUS_CODE_CDED0_BOOT_COMPLETE
#include <bug_check.h>         // for BUG_ASSERT_PARAM
#include <fpfw_icc_base.h>     // for fpfw_icc_base_recv_req_t
#include <fpfw_icc_base_i.h>   // for fpfw_icc_base_ctx_t
#include <fpfw_init.h>         // for fpfw_init_get_handle
#include <fpfw_status.h>       // for fpfw_status_t
#include <fpfw_timer.h>        // for fpfw_timer_create, fpfw_timer_enable...
#include <fpfw_timer_port.h>   // for _fpfw_timer_t
#include <fpfw_timer_types.h>  // for FPFW_TIMER_ONESHOT
#include <icc_platform_defines.h> // for accel_boot_status_msg
#include <idsw_kng.h>             // for KNG_DIE_ID, idsw_get_die_id
#include <kng_soc_constants.h>    // for NUM_DIE
#include <stdio.h>                // for accel_boot_status_msg

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/**
 * 5 seconds is good enough time for accel cores to boot
 * TODO: Profile the boot time of accel cores during bring up
 */
#define ACCEL_BOOT_TIMEOUT_MS 5000

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

static void accel_recv_boot_status_msg_icc_cb(void* context, size_t output_size_bytes, fpfw_status_t status);

/*------------------- Declarations (Statics and globals) --------------------*/

static fpfw_timer_t accel_boot_status_timers[NUM_VALID_ACCEL_ID];
static accel_boot_status_msg msg[NUM_VALID_ACCEL_ID] = {0};
static accel_boot_status_msg ack[NUM_VALID_ACCEL_ID] = {0};
static accel_cd_params recv_params[NUM_VALID_ACCEL_ID];
static fpfw_icc_base_send_rsp_t send_params[NUM_VALID_ACCEL_ID];
static boot_status_req_t boot_status_req[NUM_VALID_ACCEL_ID] = {0};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

static void accel_get_group_and_instance(ACCEL_ID accel_type, uint8_t* group, uint8_t* instance)
{
    KNG_DIE_ID die_id = idsw_get_die_id();

    if (accel_type == ACCEL_ID_SDM)
    {
        *group = COMPONENT_GROUP_SDM;
        *instance = (die_id == DIE_0) ? SDM_PRIMARY : SDM_SECONDARY;
    }
    else
    {
        *group = COMPONENT_GROUP_CDED;
        *instance = (die_id == DIE_0) ? CDED_PRIMARY : CDED_SECONDARY;
    }
}

static fpfw_icc_base_ctx_t* accel_get_icc_ctx(ACCEL_ID accel_type)
{
    if (accel_type == ACCEL_ID_SDM)
    {
        return fpfw_init_get_handle("icc_sdm_mbx");
    }

    return fpfw_init_get_handle("icc_cded_mbx");
}

static void accel_empty_icc_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

static fpfw_status_t accel_recv_boot_status_msg(ACCEL_ID accel_type)
{
    fpfw_icc_base_ctx_t* icc_ctx = accel_get_icc_ctx(accel_type);

    recv_params[accel_type].accel_type = accel_type;
    recv_params[accel_type].params.payload_buffer = &msg[accel_type];
    recv_params[accel_type].params.buffer_size = sizeof(msg[accel_type]);
    recv_params[accel_type].params.recv_cmd_code = LARGE_FIFO_MAILBOX_MSG_BOOT_STATUS_REQ;
    recv_params[accel_type].params.cb = accel_recv_boot_status_msg_icc_cb;
    recv_params[accel_type].params.cb_ctx = &recv_params[accel_type].params;

    return fpfw_icc_base_recv(icc_ctx, &recv_params[accel_type].params);
}

static fpfw_status_t accel_send_ack(ACCEL_ID accel_type)
{
    fpfw_icc_base_ctx_t* icc_ctx = accel_get_icc_ctx(accel_type);

    send_params[accel_type].send_payload_buffer = &ack[accel_type];
    send_params[accel_type].send_buffer_size = sizeof(ack[accel_type]);
    send_params[accel_type].recv_payload_buffer = &ack[accel_type];
    send_params[accel_type].recv_buffer_size = sizeof(ack[accel_type]);
    send_params[accel_type].cb = accel_empty_icc_cb;
    send_params[accel_type].cb_ctx = NULL;

    return fpfw_icc_base_send_resp(icc_ctx, &send_params[accel_type]);
}

static void accel_boot_status_send_complete_cb(void* ctx)
{
    ACCEL_ID accel_type = (ACCEL_ID)ctx;

    /* Queue receive buffer to receive boot status code from accel core */
    accel_recv_boot_status_msg(accel_type);
    /* Send ACK back to the accel core indicating message received and send to HSP */
    accel_send_ack(accel_type);
}

static void accel_send_boot_status(ACCEL_ID accel_type, uint8_t boot_status)
{
    uint8_t group;
    uint8_t instance;

    accel_get_group_and_instance(accel_type, &group, &instance);
    boot_status_req[accel_type].cb = accel_boot_status_send_complete_cb;
    boot_status_req[accel_type].cb_ctx = (void*)accel_type;

    boot_status_notify_extd(&boot_status_req[accel_type], 0, GEN_BOOT_STATUS_EX_LED_CODE(group, MSCP_ACCEL, instance, boot_status));
}

static void accel_recv_boot_status_msg_icc_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);

    accel_cd_params* recv_params = context;
    ACCEL_ID accel_type = recv_params->accel_type;

    fpfw_timer_reset(&accel_boot_status_timers[accel_type]);
    BUG_ASSERT_PARAM(status == FPFW_STATUS_SUCCESS, status, accel_type);

    accel_boot_status_msg* msg = (accel_boot_status_msg*)recv_params->params.payload_buffer;

    /**
     * Prepare the ACK message to be sent back to the accel core.
     *
     * The ACK message is sent to the accelerator core to indicate that the boot
     * status code has been received and send to the HSP. Since the accelerator
     * core is waiting for ACK message using std. fpfw_icc_base_send_recv/_sync
     * API(s), this std. API(s) dictate that the RESP message should have the
     * same header as received from the other core.
     */
    ack[accel_type].hdr = msg->hdr;
    ack[accel_type].hdr.cmd = LARGE_FIFO_MAILBOX_MSG_BOOT_STATUS_RSP;

    // Send boot status code to HSP
    accel_send_boot_status(accel_type, msg->boot_status);
}

static void accel_boot_status_timeout_cb(void* ctx, fpfw_dur_t latency)
{
    ACCEL_ID accel_type = (ACCEL_ID)ctx;

    FPFW_UNUSED(latency);

    /* Clear any previous stale data */
    boot_status_req[accel_type].cb = NULL;
    boot_status_req[accel_type].cb_ctx = NULL;
    // Send failure boot status code to HSP
    post_led_status(&boot_status_req[accel_type], LED_STATUS_CODE_SCP_ACCEL_FAILED);
}

static fpfw_status_t accel_intr_crash_dump_collection_timer_init(ACCEL_ID accel_type)
{
    return FPFW_TIMER_CREATE_ONESHOT(&accel_boot_status_timers[accel_type], accel_boot_status_timeout_cb, (void*)accel_type);
}

/*----------------------------- Global Functions ----------------------------*/

void accel_send_led_boot_status(led_status_codes_t led_boot_status)
{
    /**
     * This boot status code is being send by SCP on its own i.e. it is not associated
     * with any accel core. As any request buffer will do use SDM's boot status request
     * buffer to send this boot status code to HSP.
     */
    ACCEL_ID accel_type = ACCEL_ID_SDM;

    /* Clear any previous stale data */
    boot_status_req[accel_type].cb = NULL;
    boot_status_req[accel_type].cb_ctx = NULL;
    post_led_status(&boot_status_req[accel_type], led_boot_status);
}

fpfw_status_t accel_setup_boot_status_code(ACCEL_ID accel_type)
{
    fpfw_status_t status;

    // Check if the accelerator type is valid
    if (accel_type >= NUM_VALID_ACCEL_ID)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }

    // Register for a ICC message of boot status code coming from accel core
    status = accel_recv_boot_status_msg(accel_type);
    if (status != FPFW_STATUS_SUCCESS)
    {
        return status;
    }

    // Initialize timer which waits for boot status code coming from accel core
    status = accel_intr_crash_dump_collection_timer_init(accel_type);
    if (status != FPFW_STATUS_SUCCESS)
    {
        return status;
    }

    return FPFW_INIT_STATUS_SUCCESS;
}

fpfw_status_t accel_start_boot_status_timer(ACCEL_ID accel_type)
{
    return fpfw_timer_enable(&accel_boot_status_timers[accel_type], FPFW_DUR_MS(ACCEL_BOOT_TIMEOUT_MS));
}
