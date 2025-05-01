//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file gtimer.c
 *  This modules initializes various gtimer and system counter components
 */

/*------------- Includes -----------------*/
#include <FPFwInterrupts.h>
#include <bug_check.h>
#include <fpfw_status.h>
#include <fpfw_tmr_queue.h>
#include <gtimer.h>
#include <gtimer_prodfw.h>
#include <idhw.h>                          // for idhw_is_single_die_boot_en
#include <idsw_kng.h>                      //for CPU_SCP
#include <mscp_exp_spi_synchronize_dies.h> // for mscp_exp_spi_synchronize_dies
#include <prodfw_fnc_pointers.h>
#include <stdint.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static fpfw_tmr_queue_t s_timer_queue;
static uintptr_t s_timer_base_address;
static int s_timer_irq;

/*------------- Functions ----------------*/
/**
 * Get the current counter value of the gtimer
 *
 * @return  current counter value of the gtimer
 */
uint64_t gtimer_prodfw_get_counter()
{
    return gtimer_get_counter(s_timer_base_address);
}

static void timer_rearm()
{
    uint64_t next_tick;

    FPFwCoreInterruptDisableVector(s_timer_irq);
    fpfw_tmr_queue_expire(&s_timer_queue, &next_tick);
    gtimer_set_timer(s_timer_base_address, next_tick);
    FPFwCoreInterruptEnableVector(s_timer_irq);
}

static void timer_isr(void* ctx_ptr)
{
    FPFW_UNUSED(ctx_ptr);

    uint64_t next_tick;
    fpfw_tmr_queue_expire(&s_timer_queue, &next_tick);
    gtimer_set_timer(s_timer_base_address, next_tick);
}

void gtimer_prodfw_init(gtimer_prodfw_init_config_t* config)
{
    BUG_ASSERT(config != NULL);

    printf("[GTimer] Initializing gtimer with config:\n");
    printf("  counter_control_base: 0x%x\n", config->counter_control_base);
    printf("  timer_control_base:   0x%x\n", config->timer_control_base);
    printf("  timer_base_address:   0x%x\n", config->timer_base_address);
    printf("  frequency_hz:         %" PRIu32 "\n", config->frequency_hz);
    printf("  scaling_factor:       %d\n", config->scaling_factor);
    printf("  timer_irq:            %" PRIu32 "\n", config->timer_irq);
    s_timer_base_address = config->timer_base_address;
    s_timer_irq = config->timer_irq;

    if (idsw_get_cpu_type() == CPU_SCP)
    {
        if (!idhw_is_single_die_boot_en())
        {
            //! SCP 0 & 1 must synchronize before system counter init & configuring the d2d counters
            int d2d_sync_cntr_status = mscp_exp_spi_synchronize_dies(d2d_sync_point_sys_cnt, idsw_get_die_id());
            BUG_ASSERT_PARAM(d2d_sync_cntr_status == SILIBS_SUCCESS, d2d_sync_cntr_status, SILIBS_SUCCESS);
        }
        //! Initialize & enable generic system counter & update frequency
        system_counter_init(config->counter_control_base, config->frequency_hz, config->scaling_factor);
    }
    //! Initialize & enable counter for the given device & update frequency
    gtimer_init(config->timer_control_base);
    gtimer_enable_timer(config->timer_base_address);

    fpfw_status_t status = fpfw_tmr_queue_init(&s_timer_queue, gtimer_prodfw_get_counter);
    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);

    int intr_init_status =
        FPFwCoreInterruptRegisterCallback(config->timer_irq, (FPFwCoreInterruptHandler)timer_isr, (void*)&s_timer_queue);
    BUG_ASSERT(intr_init_status == FPFW_STATUS_SUCCESS);

    intr_init_status = FPFwCoreInterruptEnableVector(s_timer_irq);
    BUG_ASSERT(intr_init_status == FPFW_STATUS_SUCCESS);
    silibs_platform_set_fnc_pointer(gtimer_prodfw_get_frequency, gtimer_prodfw_get_counter);
}

void gtimer_add_oneshot(fpfw_tmr_entry_t* tmr, uint64_t tick_interval, void (*cb)(void*, uint64_t, uint64_t), void* ctx)
{
    BUG_ASSERT(fpfw_tmr_queue_add_oneshot(&s_timer_queue, tmr, tick_interval, cb, ctx) == FPFW_STATUS_SUCCESS);

    timer_rearm();
}

void gtimer_add_periodic(fpfw_tmr_entry_t* tmr, uint64_t tick_interval, void (*cb)(void*, uint64_t, uint64_t), void* ctx)
{
    BUG_ASSERT(fpfw_tmr_queue_add_periodic(&s_timer_queue, tmr, gtimer_prodfw_get_counter(), tick_interval, cb, ctx) ==
               FPFW_STATUS_SUCCESS);

    timer_rearm();
}

void gtimer_remove(fpfw_tmr_entry_t* tmr)
{
    BUG_ASSERT(fpfw_tmr_queue_rmv(&s_timer_queue, tmr) == FPFW_STATUS_SUCCESS);

    timer_rearm();
}

/**
 * Get the frequency of the gtimer
 *
 * @return  frequency of the gtimer
 */
uint32_t gtimer_prodfw_get_frequency()
{
    return gtimer_get_frequency(s_timer_base_address);
}