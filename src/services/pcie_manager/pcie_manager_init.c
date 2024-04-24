//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Initializes SCP service responsible for PCIe root port subsystem
 * management.
 */

/*------------- Includes -----------------*/
#include <DfwkThreadXHost.h>
#include <ErrorHandler.h>
#include <debug.h>
#include <errno.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <pcie_manager_i.h>
#include <scp_pcie_manager.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>
#include <tx_initialize.h>
#include <tx_thread.h>

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
void scp_pcie_initialize(PDFWK_SCHEDULE schedule)
{
    if (schedule == NULL)
    {
        printf("%s: Failure - NULL driver framework schedule!\n", __func__);
        FPFwErrorRaise(-EINVAL);
    }

    for (RPSS_INSTANCE i = RPSS0; i < PCIE_RPSS_COUNT; i++)
    {
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
            FPFwErrorRaise(status);
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
            FPFwErrorRaise(status);
        }
    }
}
