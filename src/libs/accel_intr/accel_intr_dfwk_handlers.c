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
#include <accelip_id.h>        // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <atu_init.h>          // for atu_svc_accel_atu_addr
#include <bug_check.h>         // for BUG_CHECK_EXTERNAL
#include <cortex_m7_atomics.h> // for cortex_m7_atomic_call_data_memory_barrier
#include <crash_dump.h>        // for crash_dump_is_accel_cd_complete
#include <fpfw_timer.h>        // for fpfw_timer_create, fpfw_timer_enable...
#include <fpfw_timer_port.h>   // for fpfw_timer_create, fpfw_timer_enable...
#include <sdm_ext_cfg_regs.h>  // for _addressblock_0x100000_misc_sys_ext_intr2_msg_send_intr
#include <stdbool.h>           // for true, false

/*-------------------- Symbolic Constant Macros (defines) -------------------*/
/**
 * This is the period value passed when crating timer.
 * This is not used for Oneshot timer and needs to be kept 0.
 */
#define ACCEL_INTR_HANDSHAKE_TIMER_PERIOD_UNUSED_FOR_ONESHOT (0)

/**
 * For One-shot timer this is the time after which timer will expire
 * Currently we are setting this to 100 timer ticks which is 1 second
 * Reference: tx_initialised_low_level.S from Cortex_m7 repo
 * TODO: Task 1974315: [SCP] Update timeout in SCP for Crash dump collection in SDM
 */
#define ACCEL_INTR_HANDSHAKE_TIMER_DELAY_IN_NUMBER_OF_TICKS (FPFW_TIMER_TX_TICK_PERIOD_NS * 100)

/**
 * Pass reset request selection to SDM_MSG handler function
 */
#define ACCEL_INTR_REQUEST_SOC_RESET         (true)
#define ACCEL_INTR_REQUEST_ACCEL_EMCPU_RESET (false)

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
    {.accel_type = ACCEL_ID_SDM, .is_soc_reset = false, .is_collecting_crashdump = false},

    // ACCEL_ID_CDED
    {.accel_type = ACCEL_ID_CDED, .is_soc_reset = false, .is_collecting_crashdump = false}};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/**
 * @brief Request crash dump collection from Accel emCPU
 *
 * @details This function will perform following tasks
 * 1. Create timer to wait on doorbell interrupt SDM_MSG0_INTR using fpfw_timer_create
 * 2. Enable IRQ from ACCEL IP
 * 3. Send request to ACCEL emCPU to collect crash dump. This is done by raising doorbell interrupt SYS2_MSG0_INTR
 *
 * @param[in] accel_type : Accel id for CDED / SDM
 *
 * @retval void
 *
 */
static void accel_intr_request_crash_dump_collection(ACCEL_ID accel_type, bool is_soc_reset)
{
    /**
     * is_collecting_crashdump is set, indicates
     * We started crash dump collection for a particular FATAL interrupt and have received another fatal interrupt
     * 1. We will not start new crash dump collection
     * 2. We will check if soc_reset is needed and will mark that in timer context
     * 3. We will enable NVIC interrupt incase it has been disabled as part of FATAL ISR handler
     */
    /**
     * Update this flow as per following ADO point 5
     * TODO: Task 1982366: [SCP] Accel IP Fatal Interrupt Cleanup Tasks / Comments
     * When one FATAL interrupt is received ->
     * Halt the processor where it should be in WFI (waiting for interrupt) state ->
     * Only handles Doorbell for crash sump collection
     */
    if (accel_intr_crash_dump_collection_timer_data[accel_type].is_collecting_crashdump)
    {
        accel_intr_crash_dump_collection_timer_data[accel_type].is_soc_reset |= is_soc_reset;
        return;
    }

    // Set timeout count
    accel_intr_crash_dump_collection_timer_data[accel_type].is_soc_reset = is_soc_reset;
    accel_intr_crash_dump_collection_timer_data[accel_type].is_collecting_crashdump = true;

    // Enable timer
    fpfw_timer_enable(&accel_intr_crash_dump_collection_timers[accel_type], ACCEL_INTR_HANDSHAKE_TIMER_DELAY_IN_NUMBER_OF_TICKS);
}

/**
 * @brief Callback function for FPFW timer
 *
 * @param[in] ctx : context pointer passed into the callback as arg
 * @param[in] latency : delay in time units between expected start of callback and empirical start time
 * @retval void
 *
 */
static void accel_intr_handle_sdm_msg_recv_timeout(void* ctx, fpfw_dur_t latency)
{
    FPFW_UNUSED(latency);

    paccel_intr_crash_dump_collection_timer_data_t timer_ctx = ctx;
    ACCEL_ID accel_type = timer_ctx->accel_type;

    FPFW_ET_LOG(AccelIntrCrashdumpCollectTimeout, accel_type);

    /**
     * 1. TODO: Task 1908548: [SCP] Implementation of SDM Fatal Interrupt in SCP
     * Reset Accel emCPU and wait for emCPU to bootup
     */

    if (timer_ctx->is_soc_reset)
    {
        FPFW_ET_LOG(AccelIntrSoCReset, accel_type);
    }
    else
    {
        FPFW_ET_LOG(AccelIntremCPUReset, accel_type);
    }

    if (crash_dump_is_accel_cd_complete(accel_type))
    {
        // Crash SCP
        BUG_CHECK_EXTERNAL();
    }
    else
    {
        /**
         * As per design we should warm reset accel core in this path.
         * TODO:
         * 1) Warm reset accel core
         * 2) Collect crashdump
         * 3) Crash SCP
         */
        BUG_CHECK_EXTERNAL();
    }
}

/*----------------------------- Global Functions ----------------------------*/

uint32_t accel_intr_crash_dump_collection_timer_init(ACCEL_ID accel_type)
{
    fpfw_status_t status = fpfw_timer_create(&accel_intr_crash_dump_collection_timers[accel_type],
                                             FPFW_TIMER_ONESHOT,
                                             ACCEL_INTR_HANDSHAKE_TIMER_PERIOD_UNUSED_FOR_ONESHOT,
                                             (fpfw_timer_callback)accel_intr_handle_sdm_msg_recv_timeout,
                                             &accel_intr_crash_dump_collection_timer_data[accel_type]);

    if (status != FPFW_STATUS_SUCCESS)
    {
        critical_print("fpfw_timer_create failed(0x%x)\n", status);
        return ACCEL_INTR_RET_FAIL_TIMER_CREATE;
    }

    return ACCEL_INTR_RET_SUCCESS;
}

void accel_intr_handle_fatal_intr_recvd(ACCEL_ID accel_type)
{
    // Based on ATU MAP get sdm_ext_cfg base address
    uint32_t IRQnum = accel_intr_get_irq_num_from_accel_type(accel_type);
    uint32_t ext_cfg_addr = atu_svc_accel_atu_addr(accel_type);

    /**
     * 1. Loop through all FATAL interrupts to note and log triggered interrupts.
     *    Also set flags soc_reset / accel_emcpu_reset accordingly. These are returned as part of irq_status
     */
    uint32_t irq_status =
        accel_intr_process_fatal_interrupts(IRQnum, ext_cfg_addr, ACCEL_INTR_PROCESS_INTR_IN_BOTTOM_HALF);
    /**
     * TODO: Task 1973334: [SCP] Read and dump Accel IP registers from SCP
     */

    if (ACCEL_INTR_IS_RESET_SOC_SET(irq_status))
    {
        /**
         * Request SoC reset in SCP. This should also trigger crash dump collection for ACCEL emCPU and other
         * cores. Add logic to request SoC Reset in SCP ->
         * TODO: Task 1908548: [SCP] Implementation of SDM Fatal Interrupt in SCP
         * TODO: Task 1973282: [SCP] Interface with crash dump collection in SCP
         * Remove this line when adding support for SoC reset
         */
        accel_intr_request_crash_dump_collection(accel_type, ACCEL_INTR_REQUEST_SOC_RESET);
    }
    else if (ACCEL_INTR_IS_RESET_ACCEL_EMCPU_SET(irq_status))
    {
        /**
         * 1. Send request to ACCEL emCPU to collect crash dump. This is done by raising doorbell interrupt SYS2_MSG0_INTR
         * 2. Enable IRQ from ACCEL IP
         * 3. Create timer to wait on doorbell interrupt SDM_MSG0_INTR using fpfw_timer_create
         */
        accel_intr_request_crash_dump_collection(accel_type, ACCEL_INTR_REQUEST_ACCEL_EMCPU_RESET);
    }
    else
    {
        /**
         * 1. Re-init all interrupts routed from ACCEL IP and supported in SCP in level 1 registers
         * Level 2 registers are cleared as part of Accel emCPU boot up
         */
        accel_intr_scp_init(accel_type, ext_cfg_addr);
        FPFwCoreInterruptEnableVector(IRQnum);
    }
}
