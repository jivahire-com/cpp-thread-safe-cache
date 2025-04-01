//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file boot_status.c
 * Implements Boot Status APIs
 */

/*------------- Includes -----------------*/
#include <FpFwLock.h> // for FPFW_LOCK, FpFwLockInitialize, FpFwLockAcquire, FpFwLockRelease
#include <assert.h>
#include <boot_status.h>                    // for boot_status_notify_complete, system_info_boot_status_notify
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
/**
 * @brief Memory required to raise a sync/async boot status
 * notify request to HSP.
 * Only a single request is supported at a time, so no need
 * to maintain a list of requests
 */
typedef struct _boot_status_req_t
{
    bool in_use;                              //! flag to indicate if the request is in use or not
    kng_hsp_mailbox_msg msg;                  //! actual mbox payload
    fpfw_icc_base_send_req_t hsp_send_params; //! async message send params
} boot_status_req_t;

/**
 * @brief Enum to describe the type of boot status
 *
 */
typedef enum _boot_status_type_t
{
    BOOT_STATUS_PROGRESS = 0x00,
    BOOT_STATUS_ERROR = 0x01,
} boot_status_type_t;

/**
 * @brief Boot status table to map boot status codes to their types
 *
 */
typedef struct _boot_status_table_t
{
    boot_status_code_t status_code;
    boot_status_type_t status_type;
} boot_status_table_t;

//! Update this table as new boot status codes are added
static const boot_status_table_t boot_status_table[] = {
    {BOOT_STATUS_CODE_SCP_OK, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_START, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_COLD_BOOT, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_WARM_BOOT, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_IRQ_DISABLED, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_MESH_INIT_START, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_MESH_INIT_END, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_TOWER_INIT_START, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_TOWER_INIT_END, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_ACCEL_INIT_START, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_ACCEL_INIT_END, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_DDR_INIT_START, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_DDR_INIT_END, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_E_DDR0_TRAINING, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_E_DDR1_TRAINING, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_E_DDR2_TRAINING, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_E_DDR3_TRAINING, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_E_DDR4_TRAINING, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_E_DDR5_TRAINING, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_E_DDR6_TRAINING, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_E_DDR7_TRAINING, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_E_DDR8_TRAINING, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_E_DDR9_TRAINING, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_E_DDR10_TRAINING, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_E_DDR11_TRAINING, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_SCP_MAX, BOOT_STATUS_ERROR},

    {BOOT_STATUS_CODE_MCP_OK, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_MCP_START, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_MCP_COLD_BOOT, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_MCP_WARM_BOOT, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_MCP_IRQ_DISABLED, BOOT_STATUS_PROGRESS},
    {BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG, BOOT_STATUS_ERROR},
    {BOOT_STATUS_CODE_MCP_MAX, BOOT_STATUS_ERROR},

    {BOOT_STATUS_CODE_MAX, BOOT_STATUS_ERROR}};

/*-- Symbolic Constant Macros (defines) --*/
#define BOOT_STATUS_TABLE_SIZE (sizeof(boot_status_table) / sizeof(boot_status_table[0]))

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static fpfw_icc_base_ctx_t* icc_ctx = NULL;
static boot_status_req_t req_mem = {0};
static FPFW_LOCK boot_status_lock; //! lock for protecting req_mem

/*------------- Functions ----------------*/
void boot_status_init(fpfw_icc_base_ctx_t* icc_base_ctx)
{
    icc_ctx = icc_base_ctx;
    memset(&req_mem, 0, sizeof(req_mem));
    //! Initialize the lock to protect the global memory ctx
    FpFwLockInitialize(&boot_status_lock);
}

static void boot_status_notify_complete(void* context, fpfw_status_t status)
{
    boot_status_req_t* req = (boot_status_req_t*)context;
    assert(status == FPFW_STATUS_SUCCESS); //! status of the icc base communication
    assert(req != NULL);
    assert(req->msg.header.cmd == HSP_MAILBOX_CMD_BOOT_STATUS_EXTD_NOTIFY); //! msg is a boot status notify message

    //! Print the boot status notification message on uart
    printf("[Boot Status] Async Notif Send Complete, ID[%" PRId32 "] Stat[0x%" PRIx32 "] Stat_ex[0x%" PRIx32 "]\n",
           req->msg.boot_stat_notif.id,
           req->msg.boot_stat_notif.boot_status,
           req->msg.boot_stat_notif.boot_status_ex.boot_status_int);

    //! mark the request as not in use, so the memory can be reused for next request
    FPFW_LOCK_STATE oldState = FpFwLockAcquire(&boot_status_lock); //! critical section start
    req->in_use = false; //! mark the request as not in use, so the mem can be reused for next request
    memset(&req_mem, 0, sizeof(req_mem));         //! reset the request memory just to be safe
    FpFwLockRelease(&boot_status_lock, oldState); //! critical section end
}

fpfw_status_t boot_status_notify(boot_status_code_t boot_status)
{
    KNG_CPU_TYPE cpu_type = idsw_get_cpu_type(); //! get the cpu type, scp/mcp
    fpfw_status_t status = FPFW_STATUS_SUCCESS;
    bool is_icc_sync = false;     //! cache the request type in case `is_runtime` flag updates
    bool ongoing_request = false; //! cache the in_use flag to check if the global memory is in use

    //! Check boot status validity as per core
    if (((cpu_type == CPU_SCP) && ((boot_status >= BOOT_STATUS_CODE_SCP_MAX) || (boot_status < BOOT_STATUS_CODE_SCP_OK))) ||
        ((cpu_type == CPU_MCP) && ((boot_status >= BOOT_STATUS_CODE_MCP_MAX) || (boot_status < BOOT_STATUS_CODE_MCP_OK))))
    {
        return FPFW_STATUS_INVALID_ARGS;
    }

    //! Check if the pre requisite for sending boot status notify is met. hsp is present and icc ctx is set
    if ((!system_info_is_hsp_present()) || (icc_ctx == NULL))
    {
        return FPFW_STATUS_INVALID_STATE;
    }

    //! check if there is already an ongoing request, only a single request is supported at a time
    //! Only applicable for async requests raised from different threads during runtime
    FPFW_LOCK_STATE oldState = FpFwLockAcquire(&boot_status_lock); //! critical section start
    if (req_mem.in_use)
    {
        ongoing_request = true; //! global memory is in use, return busy status
    }
    else
    {
        //! mark the request as in use
        req_mem.in_use = true;
    }
    FpFwLockRelease(&boot_status_lock, oldState); //! critical section end
    if (ongoing_request)
    {
        return FPFW_STATUS_BUSY;
    }

    //! Prepare the hsp message packet
    req_mem.msg.boot_stat_notif.header.cmd = HSP_MAILBOX_CMD_BOOT_STATUS_EXTD_NOTIFY;
    req_mem.msg.boot_stat_notif.id = (cpu_type == CPU_SCP) ? HSP_FIRMWARE_ID_SCP : HSP_FIRMWARE_ID_MCP;
    req_mem.msg.boot_stat_notif.boot_status = boot_status;
    req_mem.msg.boot_stat_notif.boot_status_ex.boot_status_int = HSP_MAILBOX_BOOT_STATUS_EX_FATAL; //! Set by default
    //! iterate table of boot status codes to find the boot status type to update boot_status_ex
    for (uint32_t i = 0; i < BOOT_STATUS_TABLE_SIZE; i++)
    {
        if (boot_status == boot_status_table[i].status_code)
        {
            if (boot_status_table[i].status_type == BOOT_STATUS_PROGRESS)
            {
                req_mem.msg.boot_stat_notif.boot_status_ex.boot_status_int = HSP_MAILBOX_BOOT_STATUS_EX_COMPLETE;
            }
            break;
        }
    }

    //! Check current state of the system, init or runtime
    if (system_info_is_init_complete())
    {
        //! Prepare async send request
        req_mem.hsp_send_params.payload_buffer = &req_mem.msg;
        req_mem.hsp_send_params.buffer_size = sizeof(req_mem.msg);
        req_mem.hsp_send_params.cb = boot_status_notify_complete;
        req_mem.hsp_send_params.cb_ctx = &req_mem;

        //! Raise an async hsp mbox send request to notify hsp of boot status
        //! Hsp will not send a response for this message, so no need to wait for a response
        status = fpfw_icc_base_send(icc_ctx, &req_mem.hsp_send_params);
        //! Clear up the memory for next request if failure
        if (status != FPFW_STATUS_SUCCESS)
        {
            FPFW_LOCK_STATE oldState = FpFwLockAcquire(&boot_status_lock); //! critical section start
            req_mem.in_use = false;
            memset(&req_mem, 0, sizeof(req_mem));
            FpFwLockRelease(&boot_status_lock, oldState); //! critical section end
        }
    }
    else
    {
        //! Raise sync hsp mbox request to notify hsp of boot status, blocking call, will wait until send is
        //! complete, ie data pushed to fifo or timeout occurs
        status = fpfw_icc_base_send_sync(icc_ctx, &req_mem.msg, sizeof(req_mem.msg));
        is_icc_sync = true;
        //! mark the request as not in use, no need for locks as we are still in init phase
        req_mem.in_use = false;
        //! reset the request memory
        memset(&req_mem, 0, sizeof(req_mem));
    }

    if (status != FPFW_STATUS_SUCCESS)
    {
        printf("[Boot Status] %s Notif Send Failed, Status[0x%" PRIx32 "]\n", ((is_icc_sync) ? "Sync" : "Async"), status);
    }
    else
    {
        printf("[Boot Status] %s Notif Send %s, ID[%" PRId32 "] Stat[0x%" PRIx32 "] Stat_ex[0x%" PRIx32 "]\n",
               ((is_icc_sync) ? "Sync" : "Async"),
               ((is_icc_sync) ? "Completed" : "Raised"),
               req_mem.msg.boot_stat_notif.id,
               req_mem.msg.boot_stat_notif.boot_status,
               req_mem.msg.boot_stat_notif.boot_status_ex.boot_status_int);
    }
    return status;
}