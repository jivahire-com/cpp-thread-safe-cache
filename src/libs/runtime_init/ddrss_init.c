//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddrss_init.c
 * This file contains an initialization API for the DDR Manager to be called by FPFW_INIT_COMPONENT
 *  DDRSS Library init has moved to inside this init
 */

/*------------- Includes -----------------*/
#include <ddr_manager.h>
#include <ddrss.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define UNUSED(x) (void)(x)
#define KB        (1024)

#define MS_PER_TICK             (10) // Todo:  Revisit this if/when we change from ThreadX SW timer to gtimer
#define DDR_TIMER_INITIAL_TICKS (18) // Let things settle down before starting the timer
#define DDR_STACK_SIZE          ((TX_MINIMUM_STACK) + ((2) * (KB)))
#define DDR_THREAD_PRIORITY     (15)

/*-------------- Functions ---------------*/
// Todo: Add "ddr_training" to dependencies when available
FPFW_INIT_COMPONENT(ddr_pcr, FPFW_INIT_DEPENDENCIES("css_pome", "atu_svc", "hw_ver"))
{
    KNG_DIE_ID die_num = idsw_get_die_id();
    printf("DDR PCR init, die_num: [%u]\n", die_num);

    printf("DDR PCR init, die_num: [%u]\n", die_num);
    prod_ddrss_pcr_init(die_num);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(ddr, FPFW_INIT_DEPENDENCIES("std_io", "ddr_pcr", "mesh", "hw_ver", "icc_hspmbx", "cfg_mgr", "css_pome", "atu_svc", "fuse_post_mesh"))
{
    static uint8_t ddr_stack[DDR_STACK_SIZE];
    static uint32_t ddr_queue_pool[10];
    static ddr_service_context_t ddr_service_ctx = {0};

    ddr_service_config_t config = {.thread_config =
                                       {
                                           .p_stack = ddr_stack,
                                           .stack_size = sizeof(ddr_stack),
                                           .priority = DDR_THREAD_PRIORITY,
                                           .time_slice_option = TX_NO_TIME_SLICE,
                                           .die_number = idhw_get_die_id(),
                                       },
                                   .timer_config =
                                       {
                                           .initial_ticks = DDR_TIMER_INITIAL_TICKS,
                                           .reschedule_ticks = config_get_ddrmanager_bwl_polling_period_ms() / MS_PER_TICK,
                                       },
                                   .queue_config = {
                                       .p_queue = ddr_queue_pool,
                                       .msg_size = sizeof(ddr_queue_pool[0]) / sizeof(uint32_t),
                                       .queue_num_words = sizeof(ddr_queue_pool) / sizeof(uint32_t),
                                   }};

    // Load PHY binaries
    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");
    if (icc_ctx == NULL)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_E_POINTER, "Failed to get icc_hspmbx handle - Cannot init ddrss"};
    }

    hsp_send_recv_load_fw_ddr_phy_req(icc_ctx);

    // Initialize DDR Manager - prod_ddrss_lib_init is moved to inside this init
    ddr_manager_init(&ddr_service_ctx, &config, icc_ctx);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}