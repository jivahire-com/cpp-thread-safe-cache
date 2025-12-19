//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_boot_status.c
 * Accelerator cores boot status code APIs
 */

/*-------------------------------- Includes ---------------------------------*/
#include "accelerator_ip.h"
#include "accelerator_ip_events.h"
#include "accelerator_ip_priv.h"

#include <DbgPrint.h>          // for FPFW_DBGPRINT_INFO, FPFW_DBGPRINT_ERROR
#include <DfwkDriver.h>        // for DfwkAsyncRequestComplete
#include <FpFwUtils.h>         // for FPFW_DUR_S
#include <accelip_id.h>        // for NUM_VALID_ACCEL_ID, ACCEL_ID_SDM
#include <boot_status.h>       // for boot_status_notify_extd
#include <boot_status_codes.h> // for BOOT_STATUS_CODE_SDM0_BOOT_COMPLETE, BOOT_STATUS_CODE_CDED0_BOOT_COMPLETE
#include <bug_check.h>         // for BUG_ASSERT_PARAM
#include <fpfw_cfg_mgr.h>      // for config_get_accel_boot_timeout_sec
#include <fpfw_icc_base.h>     // for fpfw_icc_base_recv_req_t
#include <fpfw_icc_base_i.h>   // for fpfw_icc_base_ctx_t
#include <fpfw_init.h>         // for fpfw_init_get_handle
#include <fpfw_status.h>       // for fpfw_status_t
#include <fpfw_timer.h>        // for fpfw_timer_create, fpfw_timer_enable...
#include <fpfw_timer_port.h>   // for _fpfw_timer_t
#include <fpfw_timer_types.h>  // for FPFW_TIMER_ONESHOT
#include <gtimer_prodfw.h>     // for gtimer_get_timestamp_us
#include <icc_platform_defines.h> // for accel_boot_status_msg
#include <idsw_kng.h>             // for KNG_DIE_ID, idsw_get_die_id
#include <kng_soc_constants.h>    // for NUM_DIE
#include <stdio.h>                // for accel_boot_status_msg
#include <tx_api.h>               // for tx_semaphore_create, tx_semaphore_put, tx_semaphore_get

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/**
 * Given that SCP FW is busy handling other tasks, 20 seconds is good enough
 * time for SCP FW to record that accel cores have booted
 * After profiling it turns out accel cores take less then a second to boot
 */
#define ACCEL_BOOT_TIMEOUT_S  (config_get_accel_boot_timeout_sec())
#define ACCEL_BOOT_TIMEOUT_MS FPFW_DUR_S_TO_MS(ACCEL_BOOT_TIMEOUT_S)
#define ACCEL_BOOT_TIMEOUT_US FPFW_DUR_S_TO_US(ACCEL_BOOT_TIMEOUT_S)

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
static uint64_t boot_start_ts_us[NUM_VALID_ACCEL_ID] = {0};
static TX_SEMAPHORE s_accel_boot_sem[NUM_VALID_ACCEL_ID] = {0};
static char* s_accel_boot_sem_name[NUM_VALID_ACCEL_ID] = {"sdm_boot_sem", "cded_boot_sem"};
static PDFWK_ASYNC_REQUEST_HEADER s_sos_dfwk_boot_sync_req[NUM_VALID_ACCEL_ID] = {NULL};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

static void accel_boot_status_put_sem(ACCEL_ID accel_type)
{
    unsigned int status = tx_semaphore_put(&s_accel_boot_sem[accel_type]);
    BUG_ASSERT_PARAM(status == TX_SUCCESS, status, accel_type);
}

static unsigned int accel_boot_status_get_sem(ACCEL_ID accel_type)
{
    unsigned int status = tx_semaphore_get(&s_accel_boot_sem[accel_type], TX_NO_WAIT);
    BUG_ASSERT_PARAM(status == TX_SUCCESS || status == TX_NO_INSTANCE, status, accel_type);

    return status;
}

static void accel_log_boot_time(ACCEL_ID accel_type)
{
    uint64_t boot_time = gtimer_get_timestamp_us() - boot_start_ts_us[accel_type];
    KNG_DIE_ID die_id = idsw_get_die_id();
    uint8_t valid_flag = boot_time <= ACCEL_BOOT_TIMEOUT_US ? 1 : 0;

    FPFW_ET_LOG(AccelBootTime, boot_time, accel_type, die_id, valid_flag);
}

static bool accel_is_ok_bsc(uint8_t boot_status_code)
{
    if (boot_status_code == BOOT_STATUS_CODE_SDM0_OK || boot_status_code == BOOT_STATUS_CODE_SDM1_OK ||
        boot_status_code == BOOT_STATUS_CODE_CDED0_OK || boot_status_code == BOOT_STATUS_CODE_CDED1_OK)
    {
        return true;
    }

    return false;
}

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

static void accel_boot_failed_complete_cb(void* ctx)
{
    ACCEL_ID accel_type = (ACCEL_ID)ctx;
    uint32_t error_code = accel_type == ACCEL_ID_SDM ? KNG_HM_SDM_BOOT_FAILED : KNG_HM_CDED_BOOT_FAILED;

    BUG_CHECK(error_code, accel_type, 0);
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

    /* Complete any outstanding SOS DFWK request hanging for this accel core */
    if (s_sos_dfwk_boot_sync_req[accel_type])
    {
        DfwkAsyncRequestComplete(s_sos_dfwk_boot_sync_req[accel_type]);
        s_sos_dfwk_boot_sync_req[accel_type] = NULL;
    }
    else
    {
        /**
         * Availability of accel boot status semaphore indicates that it has been booted.
         * So once you get a boot status notification from accel core put this semaphore.
         * This semaphore will be later read by SOS service to sync for accel boot complete.
         */
        accel_boot_status_put_sem(accel_type);
    }

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

    // Log the boot time taken by accel core
    if (accel_is_ok_bsc(msg->boot_status))
    {
        accel_log_boot_time(accel_type);
    }

    // Send boot status code to HSP
    accel_send_boot_status(accel_type, msg->boot_status);
}

static void accel_boot_status_timeout_cb(void* ctx, fpfw_dur_t latency)
{
    ACCEL_ID accel_type = (ACCEL_ID)ctx;

    FPFW_UNUSED(latency);

    /* BUG_CHECK() after sending boot status code to HSP */
    boot_status_req[accel_type].cb = accel_boot_failed_complete_cb;
    boot_status_req[accel_type].cb_ctx = (void*)accel_type;
    // Send failure boot status code to HSP
    post_led_status(&boot_status_req[accel_type], LED_STATUS_CODE_SCP_ACCEL_FAILED);
}

static fpfw_status_t accel_boot_status_timer_init(ACCEL_ID accel_type)
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
    status = accel_boot_status_timer_init(accel_type);
    if (status != FPFW_STATUS_SUCCESS)
    {
        return status;
    }

    return FPFW_INIT_STATUS_SUCCESS;
}

fpfw_status_t accel_start_boot_status_timer(ACCEL_ID accel_type)
{
    boot_start_ts_us[accel_type] = gtimer_get_timestamp_us();

    s_sos_dfwk_boot_sync_req[accel_type] = NULL;

    return fpfw_timer_enable(&accel_boot_status_timers[accel_type], FPFW_DUR_MS(ACCEL_BOOT_TIMEOUT_MS));
}

void accel_boot_status_init_sem()
{
    for (ACCEL_ID accel_type = ACCEL_ID_SDM; accel_type < NUM_VALID_ACCEL_ID; accel_type++)
    {
        unsigned int status = tx_semaphore_create(&s_accel_boot_sem[accel_type], s_accel_boot_sem_name[accel_type], 0);
        BUG_ASSERT_PARAM(status == TX_SUCCESS, status, accel_type);
    }
}

void accel_boot_status_wait_boot_complete(ACCEL_ID accel_type, PDFWK_ASYNC_REQUEST_HEADER p_request)
{
    BUG_ASSERT(p_request != NULL)
    BUG_ASSERT_PARAM(accel_type < NUM_VALID_ACCEL_ID, accel_type, 0);

    /**
     * Check for accel boot status semaphore is availability if it is available that mean that
     * accel core has been booted. In case it is not available store the SOS DFWK request and
     * complete it later when the accel core boot status is received.
     */
    unsigned int status = accel_boot_status_get_sem(accel_type);
    if (status == TX_SUCCESS)
    {
        /* Complete the SOS DFWK request as this accel boot is complete */
        DfwkAsyncRequestComplete(p_request);
    }
    else
    {
        /* Store the request and complete it later when the accel core boot status is received */
        s_sos_dfwk_boot_sync_req[accel_type] = p_request;
    }
}
