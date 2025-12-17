//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This section is used to describe the file and any relevant
 * information related to it purpose and how it should be used.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkDriver.h>
#include <DfwkPtrTypes.h>
#include <FpFwUtils.h>
#include <pcie_dfwk.h>
#include <pcie_dfwk_i.h>
#include <pcie_events.h>
#include <silibs_kng_soc.h>
#include <silibs_status.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PCIE_ASYNC_POOL_SIZE                (PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS)
#define GET_RPSS_POOL_IDX(rpss_idx, rp_idx) (((rpss_idx % PCIE_RPSS_PER_DIE) * PCIESS_NUM_PORTS) + rp_idx)

/*------------- Typedefs -----------------*/
typedef struct _pcie_async_pool_t
{
    pcie_async_request_t* req;
    TX_SEMAPHORE binary_sem;
} pcie_async_pool_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pcie_async_pool_t async_req_pool[PCIE_ASYNC_POOL_SIZE] = {0};

/*------------- Functions ----------------*/
pcie_async_request_t* get_pending_async_req_for_this_rp(uint8_t rpss_idx, uint8_t rp_idx, pcie_rp_async_request_t req_type)
{
    pcie_async_request_t* req = NULL;

    if ((rpss_idx >= PCIE_NUM_RPSS) || (rp_idx >= PCIESS_NUM_PORTS) || (req_type >= PCIE_MAX_ASYNC_REQ))
    {
        goto exit;
    }

    /*
     * Get the binary_sem without waiting as this can even be called from
     * an ISR context. If a use-case for a longer wait time arises the wait can
     * be made a parameter passed to this function.
     */
    uint8_t pool_idx = GET_RPSS_POOL_IDX(rpss_idx, rp_idx);
    unsigned int status = tx_semaphore_get(&(async_req_pool[pool_idx].binary_sem), TX_NO_WAIT);
    if (status != TX_SUCCESS)
    {
        goto exit;
    }

    req = async_req_pool[pool_idx].req;

    /* No outstanding of the chosen type exists */
    if ((req != NULL) && (req->rp_op != req_type))
    {
        req = NULL;
    }

    tx_semaphore_put(&(async_req_pool[pool_idx].binary_sem));

exit:
    return req;
}

void complete_async_req_for_this_rp(pcie_async_request_t* req)
{

    if ((req == NULL) || (req->rpss_index >= PCIE_NUM_RPSS) || (req->rp_index >= PCIESS_NUM_PORTS))
    {
        return;
    }

    uint8_t pool_idx = GET_RPSS_POOL_IDX(req->rpss_index, req->rp_index);
    unsigned int status = tx_semaphore_get(&(async_req_pool[pool_idx].binary_sem), TX_NO_WAIT);
    if (status != TX_SUCCESS)
    {
        return;
    }

    /* Complete this request only in cases where it is not null */
    if (async_req_pool[pool_idx].req != NULL)
    {
        async_req_pool[pool_idx].req = NULL;
        DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)(req));
    }

    tx_semaphore_put(&(async_req_pool[pool_idx].binary_sem));
}

unsigned int add_async_req_to_pool(pcie_async_request_t* req)
{
    uint8_t pool_idx = GET_RPSS_POOL_IDX(req->rpss_index, req->rp_index);

    unsigned int status = tx_semaphore_get(&(async_req_pool[pool_idx].binary_sem), TX_NO_WAIT);
    if (status != TX_SUCCESS)
    {
        return status;
    }

    if (async_req_pool[pool_idx].req == NULL)
    {
        async_req_pool[pool_idx].req = req;
    }
    else
    {
        status = TX_NO_MEMORY;
    }

    tx_semaphore_put(&(async_req_pool[pool_idx].binary_sem));

    return status;
}

void pcie_per_rp_dispatch(PDFWK_ASYNC_REQUEST_HEADER req, void* context)
{
    FPFW_UNUSED(context);
    pcie_async_request_t* r = (pcie_async_request_t*)req;

    if (r->rp_op < PCIE_MAX_ASYNC_REQ)
    {
        unsigned int status = add_async_req_to_pool(r);
        if (status != TX_SUCCESS)
        {
            PCIE_ET_ERROR_PARAM(PCIE_ET_TYPE_ENQUEUE_REQUEST_FAILED, status);
            FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Failed to enqueue request! Status: %d\n", r->rpss_index, r->rp_index, status);
            DfwkAsyncRequestComplete(req);
            return;
        }

        switch (r->rp_op)
        {
        case WAIT_FOR_EVENT: {
            /* Do any async request pre-processing here based on r->rp_op type if needed */
            break;
        }
        default: {
            PCIE_ET_ERROR_PARAM(PCIE_ET_TYPE_UNKNOWN_REQUEST, r->rp_op);
            FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Received an unknown request!\n", r->rpss_index, r->rp_index);
            r->status = SILIBS_E_PARAM;
            complete_async_req_for_this_rp(r);
            break;
        }
        }
    }
    else
    {
        /* Mark Request as Invalid Paramter */
        r->status = SILIBS_E_PARAM;
    }
}

void pcie_default_dispatch(PDFWK_ASYNC_REQUEST_HEADER incoming, void* context)
{
    FPFW_UNUSED(context);

    /* No null check needed as DFWK handles it for us before dispatching */

    pciess_device_interface_t* iface = (pciess_device_interface_t*)(incoming->OwningInterface);
    pciess_device_t* dev = (pciess_device_t*)(iface->dev);
    pcie_async_request_t* r = (pcie_async_request_t*)incoming;

    if (r->rp_index >= PCIESS_NUM_PORTS || r->rpss_index >= PCIE_NUM_RPSS)
    {
        PCIE_ET_ERROR_PARAM(PCIE_ET_TYPE_INVALID_RP_RANGES, r->rpss_index);
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Root port ranges invalid!\n", r->rpss_index, r->rp_index);
        r->status = SILIBS_E_PARAM;
        DfwkAsyncRequestComplete(incoming);
        return;
    }

    DfwkQueueEnqueueRequest(&(dev->per_rp_queue[r->rp_index]), incoming);
}

unsigned int init_async_request_pool()
{
    unsigned int status = 0;
    static bool pool_initialized = false;

    if (pool_initialized == true)
    {
        return TX_SUCCESS;
    }

    for (uint8_t i = 0; i < PCIE_ASYNC_POOL_SIZE; i++)
    {
        /*
         * Create a binary semaphore to guard concurrent accesses from
         * ISR/thread contexts to each request in our pool.
         * We cannot use a mutex here as threadx will not allow accesses to a
         * mutex from ISR context.
         */
        status = tx_semaphore_create(&(async_req_pool[i].binary_sem), "pcie_async_pool_binary_sem", 1);
        async_req_pool[i].req = NULL;
    }

    pool_initialized = true;
    return status;
}
