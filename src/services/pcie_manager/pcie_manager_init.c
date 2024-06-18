//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Initializes SCP service responsible for PCIe root port subsystem
 * management.
 */

/*------------- Includes -----------------*/
#include "DfwkPtrTypes.h" // for PDFWK_SCHEDULE

#include <ErrorHandler.h>      // for FPFwErrorRaise
#include <errno.h>             // for EINVAL
#include <kng_soc_constants.h> // for RPSS0, RPSS_INSTANCE
#include <pcie_dfwk.h>         // for PCIE_RPSS_COUNT, pcie_dfwk_init, pcie...
#include <pcie_manager_i.h>    // for rpss_service_thread_fn
#include <scp_pcie_manager.h>  // for pcie_manager_context_t, pciess_comple...
#include <stddef.h>            // for NULL
#include <stdint.h>            // for uint32_t, uint8_t
#include <stdio.h>             // for printf
#include <tx_api.h>            // for TX_SUCCESS, TX_AUTO_START, TX_NO_TIME...

/*-- Symbolic Constant Macros (defines) --*/
#define KB                                (1024)
#define PCIE_RP_SERVICE_THREAD_STACK_SIZE ((2) * (KB))

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pcie_manager_context_t pcie_mgr_ctx[PCIE_RPSS_COUNT] = {0};
static uint8_t stacks[PCIE_RPSS_COUNT * PCIE_RP_SERVICE_THREAD_STACK_SIZE] = {0};
static pciess_device_t dev[PCIE_RPSS_COUNT];
static pciess_device_interface_t iface[PCIE_RPSS_COUNT];

/*------------- Functions ----------------*/
void scp_pcie_initialize(PDFWK_SCHEDULE schedule, uint16_t rpss_to_init)
{
    if (schedule == NULL)
    {
        printf("%s: Failure - NULL driver framework schedule!\n", __func__);
        FPFwErrorRaise(-EINVAL, 0, 0, 0, 0);
    }

    for (RPSS_INSTANCE i = RPSS0; i < PCIE_RPSS_COUNT; i++)
    {
        if (((rpss_to_init >> i) & 0x1) != 0x1)
        {
            continue;
        }

        /* Initialize PCIe rpss drivers before starting up service threads */
        pcie_dfwk_init(&(dev[i]), schedule);

        pcie_mgr_ctx[i].rpss_idx = i;
        pcie_mgr_ctx[i].dev = &(dev[i]);
        pcie_mgr_ctx[i].iface = &(iface[i]);

        /* Setup worker queue for each rpss worker thread */
        int status = tx_queue_create(&(pcie_mgr_ctx[i].work_queue),
                                     "pcie_worker_queue",
                                     (sizeof(pciess_completion_request_t) / sizeof(uint32_t)), /* queue size is expected by ThreadX to be a multiple of a 32-bit word */
                                     &(pcie_mgr_ctx[i].cmpl_req),
                                     sizeof(pcie_mgr_ctx[i].cmpl_req));
        if (status != TX_SUCCESS)
        {
            printf("%s: Failed to create work queues! TX_STATUS: %d\n", __func__, status);
            FPFwErrorRaise(status, 0, 0, 0, 0);
        }

        /* Spawn off a thread that is responsible for managing each root port sub system */
        status = tx_thread_create(&pcie_mgr_ctx[i].worker,
                                  "pcie_worker_thread",
                                  rpss_service_thread_fn,
                                  (ULONG)(&pcie_mgr_ctx[i]),
                                  (void*)(&stacks[0] + (i * PCIE_RP_SERVICE_THREAD_STACK_SIZE)),
                                  PCIE_RP_SERVICE_THREAD_STACK_SIZE,
                                  10,
                                  10,
                                  TX_NO_TIME_SLICE,
                                  TX_AUTO_START);
        if (status != TX_SUCCESS)
        {
            printf("%s: Failed to create worker threads! TX_STATUS: %d\n", __func__, status);
            FPFwErrorRaise(status, 0, 0, 0, 0);
        }
    }
}
