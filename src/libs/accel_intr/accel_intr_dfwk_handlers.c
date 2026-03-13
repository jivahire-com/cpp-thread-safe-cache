//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_dfwk_handlers.c
 * This file contains handlers and functions used from DFWK request routines
 *
 */

/*-------------------------------- Features ---------------------------------*/

/*-------------------------------- Includes ---------------------------------*/
#include "accel_intr_events.h"
#include "accel_intr_priv.h"

#include <DfwkDriver.h>        // for DfwkInterfaceInitialize, DfwkQueueInitia...
#include <DfwkHost.h>          // for DfwkDeviceInitialize
#include <FPFwInterrupts.h>    // for FPFwCoreInterruptDisableVector
#include <FpFwUtils.h>         // for FPFW_UNUSED
#include <accelerator_ip.h>    // for accel_core_warm_reset
#include <accelip_id.h>        // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <atu_init.h>          // for atu_svc_accel_atu_addr
#include <bug_check.h>         // for BUG_CHECK_EXTERNAL
#include <cortex_m7_atomics.h> // for cortex_m7_atomic_call_data_memory_barrier
#include <crash_dump.h>        // for crash_dump_is_accel_cd_complete
#include <fpfw_timer.h>        // for fpfw_timer_create, fpfw_timer_enable...
#include <fpfw_timer_port.h>   // for fpfw_timer_create, fpfw_timer_enable...
#include <health_monitor.h>    // for hm_fetch_accel_fatal_cper
#include <idsw.h>              // for #include idsw_get_die_id
#include <sdm_ext_cfg_regs.h>  // for _addressblock_0x100000_misc_sys_ext_intr2_msg_send_intr
#include <stdbool.h>           // for true, false
#include <virt_irq.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/
/**
 * This is the period value passed when crating timer.
 * This is not used for Oneshot timer and needs to be kept 0.
 */
#define ACCEL_INTR_HANDSHAKE_TIMER_PERIOD_UNUSED_FOR_ONESHOT (0)

/**
 * For One-shot timer this is the time after which timer will expire
 * Currently we are setting this to 500 timer ticks which is 2 second
 * Reference: tx_initialised_low_level.S from Cortex_m7 repo
 * TODO: Task 1974315: [SCP] Update timeout in SCP for Crash dump collection in SDM
 */
#define ACCEL_INTR_HANDSHAKE_TIMER_DELAY_IN_NUMBER_OF_TICKS (FPFW_TIMER_TX_TICK_PERIOD_NS * 200)

/**
 * Pass reset request selection to SDM_MSG handler function
 */
#define ACCEL_INTR_REQUEST_SOC_RESET         (true)
#define ACCEL_INTR_REQUEST_ACCEL_EMCPU_RESET (false)

#define ACCEL_MAX_CD_COMPLETE_RETRIES (4)

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/**
 * Timer structure per Accel type / IRQ number
 */
static struct _fpfw_timer_t accel_intr_crash_dump_collection_timers[NUM_VALID_ACCEL_ID];

/**
 * Timer data structure per Accel type / IRQ number
 */
static accel_intr_crash_dump_collection_timer_data_t accel_intr_crash_dump_collection_timer_data[NUM_VALID_ACCEL_ID] = {
    // ACCEL_ID_SDM
    {.accel_type = ACCEL_ID_SDM, .retry_count = 0, .skip_cper = false, .skip_cd = false, .has_crashed = false, .cper_collected = false},

    // ACCEL_ID_CDED
    {.accel_type = ACCEL_ID_CDED, .retry_count = 0, .skip_cper = false, .skip_cd = false, .has_crashed = false, .cper_collected = false}};
static uint32_t s_accel_bh_irq_val[NUM_VALID_ACCEL_ID] = {0};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/**
 * @brief Callback function for FPFW timer
 *
 * @param[in] ctx : context pointer passed into the callback as arg
 * @param[in] latency : delay in time units between expected start of callback and empirical start time
 * @retval void
 *
 */
static void accel_intr_delay_cb(void* ctx, fpfw_dur_t latency)
{
    paccel_intr_crash_dump_collection_timer_data_t timer_data = ctx;
    ACCEL_ID accel_type = timer_data->accel_type;

    uint32_t interrupt_reg_addr =
        sdm_ext_get_category_status_reg_addr(timer_data->ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR);
    BUG_ASSERT(interrupt_reg_addr != SDM_EXT_INVALID_INTERRUPT_INPUT);

    uint32_t interrupt_mask_addr =
        sdm_ext_get_category_mask_reg_addr(timer_data->ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR);
    BUG_ASSERT(interrupt_mask_addr != SDM_EXT_INVALID_INTERRUPT_INPUT);

    uint32_t interrupt_reg_value =
        BITWISE_AND(MMIO_READ32(interrupt_reg_addr), BITWISE_INVERT(MMIO_READ32(interrupt_mask_addr)));

    uint32_t level1_clear_mask = SL_GET_BIT_MASK_RANGE(SDM_EXT_EMCPU_WDT_ERR_INTR, SDM_EXT_SDM_MSG3_INTR);
    uint32_t cper_cd_skip_flags =
        (SL_GET_SINGLE_BIT_MASK(SDM_EXT_LOCKUP_ERR_INT) | SL_GET_SINGLE_BIT_MASK(SDM_EXT_TCM_UE_ECC_ERR_INTR));

    /**
     * 1. If TCM UE or emcpu lock up error is received, we need to skip cper and cd collection
     * 2. If doorbell interrupt is received, we need to forward cper as fatal
     * 3. If emcpu wdt error, TCM UE or emcpu lockup is received, we need to forward cper as corrected
     */

    FPFW_UNUSED(latency);

    s_accel_bh_irq_val[accel_type] = interrupt_reg_value;

    if (interrupt_reg_value & cper_cd_skip_flags)
    {
        timer_data->skip_cper = true;
        timer_data->skip_cd = true;
    }

    /**
     * 1. Check if the cper magic number is written and if so collect the cper
     * 2. If magic number is not written, retry for a max of ACCEL_MAX_CD_COMPLETE_RETRIES
     * 3. Once either cper is collected or retries are exhausted, check if crash dump collection is complete
     * 4. Similar to cper, retry for a max of ACCEL_MAX_CD_COMPLETE_RETRIES
     * 5. Once either cd is complete or retries are exhausted, call bug_check
     * 6. Both UE fatals and emcpu fatals will both call bug_check
     */

    if (!timer_data->skip_cper)
    {
        if (!hm_collect_accel_fatal_cper(accel_type))
        {
            if (timer_data->retry_count < ACCEL_MAX_CD_COMPLETE_RETRIES)
            {
                FPFW_ET_LOG(AccelIntrCPERCollectTimeout, accel_type, timer_data->retry_count);
                // Restart the timer to wait for crash dump collection completion
                fpfw_timer_enable(&accel_intr_crash_dump_collection_timers[accel_type],
                                  ACCEL_INTR_HANDSHAKE_TIMER_DELAY_IN_NUMBER_OF_TICKS);
                goto cper_collect_failed;
            }
            FPFW_ET_LOG(AccelIntrCPERCollectFailed, accel_type);
            timer_data->skip_cper = true;
            timer_data->retry_count = 0;
        }
        else
        {
            timer_data->cper_collected = true;
        }
    }

    // If CPER was skipped or not collected, generate a default CPER
    if ((timer_data->skip_cper) && (!timer_data->cper_collected))
    {
        hm_generate_default_cper(accel_type);
        timer_data->cper_collected = true;
    }

    // NOTE: CD collection and copy is done in crash_dump_accel.c

    // Clear all level-1 interrupts as a precaution
    sdm_ext_int_mask_status_clear(timer_data->ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR, level1_clear_mask);

    // All crashes are control core resets
    // However only UE fatals interrupted using MSG0 are a SOC coldboot
    if (interrupt_reg_value & SL_GET_SINGLE_BIT_MASK(SDM_EXT_SDM_MSG0_INTR))
    {
        // Transmit cper with fatal severity
        FPFW_ET_LOG(AccelIntrSoCReset, accel_type);
    }
    else
    {
        // Transmit cper with corrected severity
        FPFW_ET_LOG(AccelIntremCPUReset, accel_type);
    }

    timer_data->has_crashed = true;
    BUG_CHECK_EXTERNAL(CRASH_DUMP_PROCESSOR_ID(idsw_get_die_id(), accel_type + CRASH_DUMP_CORE_SDM));
    /**
     * NOTE:
     * TODO ADO 3041451:
     * 1. Need to set reboot reason once crash is done
     * 2. This can be done using the error severity of the cper
     * 3. If it is a fatal cper, set reboot reason as COLD_BOOT else WARM_BOOT
     */

cper_collect_failed:
    timer_data->retry_count++;
}

/*----------------------------- Global Functions ----------------------------*/

uint32_t* accel_intr_get_bh_irq_val_addr(ACCEL_ID accel_type)
{
    return &s_accel_bh_irq_val[accel_type];
}

uint32_t accel_intr_crash_dump_collection_timer_init(ACCEL_ID accel_type)
{
    fpfw_status_t status = fpfw_timer_create(&accel_intr_crash_dump_collection_timers[accel_type],
                                             FPFW_TIMER_ONESHOT,
                                             ACCEL_INTR_HANDSHAKE_TIMER_PERIOD_UNUSED_FOR_ONESHOT,
                                             (fpfw_timer_callback)accel_intr_delay_cb,
                                             &accel_intr_crash_dump_collection_timer_data[accel_type]);

    if (status != FPFW_STATUS_SUCCESS)
    {
        FPFW_ET_LOG(AccelIntrDfwkHandlerError, accel_type, status, __LINE__);
        return ACCEL_INTR_RET_FAIL_TIMER_CREATE;
    }

    return ACCEL_INTR_RET_SUCCESS;
}

void accel_intr_handle_fatal_intr_recvd(ACCEL_ID accel_type)
{
    // Based on ATU MAP get sdm_ext_cfg base address
    uint32_t ext_cfg_addr = atu_svc_accel_atu_addr(accel_type);

    accel_intr_crash_dump_collection_timer_data[accel_type].ext_cfg_addr = ext_cfg_addr;
    // Enable timer
    fpfw_timer_enable(&accel_intr_crash_dump_collection_timers[accel_type], ACCEL_INTR_HANDSHAKE_TIMER_DELAY_IN_NUMBER_OF_TICKS);
}

bool accel_intr_get_cd_skip(ACCEL_ID accel_type)
{
    return accel_intr_crash_dump_collection_timer_data[accel_type].skip_cd;
}

void accel_intr_set_cper_cd_skip(void* callback_param)
{
    uint32_t IRQnum = GET_PHYSICAL_IRQ_FROM_VIRTUAL_IRQ((uint32_t)callback_param);
    ACCEL_ID accel_type = accel_intr_get_accel_type_from_irq_num(IRQnum);

    accel_intr_crash_dump_collection_timer_data[accel_type].skip_cper = true;
    accel_intr_crash_dump_collection_timer_data[accel_type].skip_cd = true;
}