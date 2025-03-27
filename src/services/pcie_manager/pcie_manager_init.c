//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Initializes SCP service responsible for PCIe root port subsystem
 * management.
 */

/*------------- Includes -----------------*/
#include <DfwkPtrTypes.h>
#include <ErrorHandler.h>
#include <errno.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <pcie_manager_i.h>
#include <pcie_phy_load_events.h>
#include <scp_pcie_manager.h>
#include <silibs_kng_soc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define KB                                (1024)
#define PCIE_RP_SERVICE_THREAD_STACK_SIZE ((2) * (KB))

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
TX_EVENT_FLAGS_GROUP pcie_phyfw_load_event;

static pcie_manager_context_t pcie_mgr_ctx[PCIE_RPSS_PER_DIE] = {0};
static uint8_t stacks[PCIE_RPSS_PER_DIE * PCIE_RP_SERVICE_THREAD_STACK_SIZE] = {0};
static pciess_device_t dev[PCIE_RPSS_PER_DIE];
static pciess_device_interface_t iface[PCIE_RPSS_PER_DIE];

static pcie_config_manager_context_t pcie_cfg_mgr_ctx = {0};
static uint8_t config_stack[PCIE_RP_SERVICE_THREAD_STACK_SIZE] = {0};
static TX_EVENT_FLAGS_GROUP event_ptr;

static kingsgate_pcie_root_bridge_config rb_config_var = {{{0}}};
static kingsgate_pcie_vab_config vab_config_var = {0};

/*------------- Functions ----------------*/

pcie_manager_context_t* scp_pcie_get_manager_context(uint8_t rpss_idx)
{
    FPFW_RUNTIME_ASSERT(rpss_idx < PCIE_NUM_RPSS);
    return &pcie_mgr_ctx[rpss_idx % PCIE_RPSS_PER_DIE];
}

/*  Initialize the event flags for configuration variables */
void scp_pcie_config_service_initialize(uint16_t rpss_to_init)
{
    if (!rpss_to_init)
    {
        printf("%s: No RPSS, not initializing PCIE config service\n", __func__);
        return;
    }

    int status = tx_event_flags_create(&event_ptr, "PCIe Config Set Event");
    if (status != TX_SUCCESS)
    {
        printf("%s: Failed to create event flags! TX_STATUS: %d\n", __func__, status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    // If this fails it asserts within the function.
    pcie_phyfw_create_event(&pcie_phyfw_load_event);

    pcie_cfg_mgr_ctx.event_ptr = &event_ptr;
    pcie_cfg_mgr_ctx.event_flags_mask = rpss_to_init;
    pcie_cfg_mgr_ctx.rb_config_var = &rb_config_var;
    pcie_cfg_mgr_ctx.vab_config_var = &vab_config_var;
}

/* Start the thread to be alerted when all configs are populated */
void scp_pcie_start_config_service_thread(void)
{
    int status = tx_thread_create(&(pcie_cfg_mgr_ctx.worker),
                                  "pcie_config_worker_thread",
                                  config_variable_service_thread_fn,
                                  (ULONG)(&pcie_cfg_mgr_ctx),
                                  (void*)(config_stack),
                                  PCIE_RP_SERVICE_THREAD_STACK_SIZE,
                                  10,
                                  10,
                                  TX_NO_TIME_SLICE,
                                  TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        printf("%s: Failed to launch config service thread! TX_STATUS: %d\n", __func__, status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }
}

void* scp_pcie_initialize(PDFWK_SCHEDULE schedule, uint16_t rpss_to_init, KNG_DIE_ID die_id)
{
    if (schedule == NULL)
    {
        printf("%s: Failure - NULL driver framework schedule!\n", __func__);
        FPFwErrorRaise(-EINVAL, 0, 0, 0, 0);
    }

    scp_pcie_config_service_initialize(rpss_to_init);

    uint16_t rpss_to_init_per_die = (die_id == DIE_1) ? (rpss_to_init >> PCIE_RPSS_PER_DIE) : (rpss_to_init);

    for (RPSS_INSTANCE i = RPSS0; i < PCIE_RPSS_PER_DIE; i++)
    {
        if (((rpss_to_init_per_die >> i) & 0x1) != 0x1)
        {
            vab_config_var.vab_config[i].vab_disable = true;
            continue;
        }

        /* Initialize PCIe rpss drivers before starting up service threads */
        pcie_dfwk_init(&(dev[i]), schedule);

        pcie_mgr_ctx[i].rpss_idx = (die_id == DIE_1) ? (i + PCIE_RPSS_PER_DIE) : i;
        pcie_mgr_ctx[i].dev = &(dev[i]);
        pcie_mgr_ctx[i].dev->rb_configs = &rb_config_var.rootbridge_config[i * PCIESS_NUM_PORTS];
        pcie_mgr_ctx[i].iface = &(iface[i]);
        pcie_mgr_ctx[i].event_ptr = &event_ptr;
        pcie_mgr_ctx[i].phyfw_load_event_ptr = &pcie_phyfw_load_event;

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

    return (void*)(&dev[0]);
}
