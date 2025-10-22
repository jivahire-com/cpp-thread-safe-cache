//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_irq_isr.c
 * This file contains ISRs for Accel IP interrupts and other validation functions
 *
 */

/*-------------------------------- Features ---------------------------------*/

/*-------------------------------- Includes ---------------------------------*/
#include "accel_intr.h"
#include "accel_intr_events.h"
#include "accel_intr_priv.h"
#include "accel_intr_service_dfwk.h"

#include <FPFwInterrupts.h>
#include <FpFwUtils.h>
#include <accel_intr_client.h>
#include <atu_init.h>
#include <health_monitor.h>
#include <stdint.h>
#include <virt_irq.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

void accel_intr_tcm_ue_emcpu_lockcup_isr(void* p_callback_param)
{
    accel_intr_set_cper_cd_skip(p_callback_param);
    accel_intr_fatal_isr(p_callback_param);
}

void accel_intr_fatal_isr(void* p_callback_param)
{
    /**
     * The fatal isr will be common as it has to only do the following:
     * 1. Disable IRQ in NVIC
     * 2. Disable Watchdog timer
     * 3. Send ASYNC message for further interrupt processing
     */

    uint32_t IRQnum = GET_PHYSICAL_IRQ_FROM_VIRTUAL_IRQ((uint32_t)p_callback_param);
    ACCEL_ID accel_type = accel_intr_get_accel_type_from_irq_num(IRQnum);
    uint32_t ext_cfg_addr = atu_svc_accel_atu_addr(accel_type);

    FPFwCoreInterruptDisableVector(IRQnum);

    accel_intr_emcpu_wdt_control(ext_cfg_addr, ACCEL_INTR_DISABLE_ACCEL_EMCPU_WDT);

    // Send ASYNC message
    send_fatal_intr_async_request(accel_type);
}
