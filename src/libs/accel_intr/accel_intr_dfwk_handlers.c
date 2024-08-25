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
#include <FPFwInterrupts.h>    // for FPFwCoreInterruptEnableVector
#include <FpFwUtils.h>         // for FPFW_UNUSED
#include <accelerator_ip.h>    // for ACCELERATOR_CDEDSS, ACCELERATOR_SDMSS
#include <cortex_m7_atomics.h> // for cortex_m7_atomic_call_data_memory_barrier
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
typedef enum
{
    ACCEL_INTR_CRASH_DUMP_COLLECT_FIRST_TIMEOUT = 0x0,
    ACCEL_INTR_CRASH_DUMP_COLLECT_SECOND_TIMEOUT,
    ACCEL_INTR_CRASH_DUMP_COLLECT_MAX
} eACCEL_INTR_CRASH_DUMP_COLLECT_STATE;

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/**
 * Timer structure per Accel type / IRQ number
 */
static struct _fpfw_timer_t accel_intr_crash_dump_collection_timers[MAX_ACCELERATOR_TYPES];

/**
 * Timer data structure per Accel type / IRQ number
 */
static accel_intr_crash_dump_collection_timer_data_t accel_intr_crash_dump_collection_timer_data[MAX_ACCELERATOR_TYPES] = {
    // ACCELERATOR_SDMSS
    {.IRQnum = SDMSS_IRQ_NUMBER,
     .timeout_count = ACCEL_INTR_CRASH_DUMP_COLLECT_FIRST_TIMEOUT,
     .is_soc_reset = false,
     .is_collecting_crashdump = false},

    // ACCELERATOR_CDEDSS
    {.IRQnum = CDEDSS_IRQ_NUMBER,
     .timeout_count = ACCEL_INTR_CRASH_DUMP_COLLECT_FIRST_TIMEOUT,
     .is_soc_reset = false,
     .is_collecting_crashdump = false}};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/**
 * @brief Resets timer used during Accel emCPU and SCP handshake
 *
 * @param[in] IRQnum : This will have IRQNum to identify if interrupt is received for CDED / SDM
 *
 * @retval void
 *
 */
static void reset_timer(uint32_t IRQnum)
{
    eACCELERATOR_TYPE accel_type = GET_ACCEL_TYPE_FROM_IRQ_NUMBER(IRQnum);

    // Reset timeout count
    accel_intr_crash_dump_collection_timer_data[accel_type].timeout_count = ACCEL_INTR_CRASH_DUMP_COLLECT_FIRST_TIMEOUT;
    accel_intr_crash_dump_collection_timer_data[accel_type].is_collecting_crashdump = false;

    // Reset timer
    fpfw_timer_reset(&accel_intr_crash_dump_collection_timers[accel_type]);
}

/**
 * @brief Raise interrupt to request crash dump collection from Accel emCPU
 *
 * @param[in] accel_type : Accelerator type SDM / CDED -> This will be used to get ATU mapped offset for register
 *
 * @retval void
 *
 */
static void accel_intr_handle_sdm_msg_send(eACCELERATOR_TYPE accel_type)
{
    // Send interrupt to Accel emCPU
    uint32_t intr2_msg_send_intr_addr =
        ACCEL_INTR_GET_DERIVED_ADDR(_ADDRESSBLOCK_0X100000_MISC_SYS_EXT_INTR2_MSG_SEND_INTR_ADDRESS,
                                    accelerator_ip_get_atu_mapped_cfg_address(accel_type));

    _addressblock_0x100000_misc_sys_ext_intr2_msg_send_intr misc_sys_ext_intr2_msg_send_intr;
    misc_sys_ext_intr2_msg_send_intr.as_uint32 = 0x0;
    misc_sys_ext_intr2_msg_send_intr.set0 = 0x1;

    MMIO_WRITE32(intr2_msg_send_intr_addr, misc_sys_ext_intr2_msg_send_intr.as_uint32);

    cortex_m7_atomic_call_data_memory_barrier();
}

/**
 * @brief Request crash dump collection from Accel emCPU
 *
 * @details This function will perform following tasks
 * 1. Create timer to wait on doorbell interrupt SDM_MSG0_INTR using fpfw_timer_create
 * 2. Enable IRQ from ACCEL IP
 * 3. Send request to ACCEL emCPU to collect crash dump. This is done by raising doorbell interrupt SYS2_MSG0_INTR
 *
 * @param[in] IRQnum : This will have IRQNum to identify if interrupt is received for CDED / SDM
 * @param[in] clear_count : Clear timeout_count in timer if set to true
 *
 * @retval void
 *
 */
static void accel_intr_request_crash_dump_collection(uint32_t IRQnum, uint32_t timeout_count, bool is_soc_reset)
{
    eACCELERATOR_TYPE accel_type = GET_ACCEL_TYPE_FROM_IRQ_NUMBER(IRQnum);

    /**
     * timeout_count is 0 and is_collecting_crashdump is set, indicates
     * We started crash dump collection for a particular FATAL interrupt and have received another fatal interrupt
     * 1. We will not start new crash dump collection
     * 2. We will check if soc_reset is needed and will mark that in timer context
     * 3. We will enable NVIC interrupt incase it has been disabled as part of FATAL ISR handler
     */
    if (timeout_count == ACCEL_INTR_CRASH_DUMP_COLLECT_FIRST_TIMEOUT &&
        accel_intr_crash_dump_collection_timer_data[accel_type].is_collecting_crashdump)
    {
        accel_intr_crash_dump_collection_timer_data[accel_type].is_soc_reset |= is_soc_reset;

        // Clear and Enable IRQ
        // Both functions always return SUCCESS
        FPFwCoreInterruptEnableVector(IRQnum);

        return;
    }

    // Set timeout count
    accel_intr_crash_dump_collection_timer_data[accel_type].timeout_count = timeout_count;
    accel_intr_crash_dump_collection_timer_data[accel_type].is_soc_reset = is_soc_reset;
    accel_intr_crash_dump_collection_timer_data[accel_type].is_collecting_crashdump = true;

    // Enable timer
    fpfw_timer_enable(&accel_intr_crash_dump_collection_timers[accel_type], ACCEL_INTR_HANDSHAKE_TIMER_DELAY_IN_NUMBER_OF_TICKS);

    // Enable SDM_MSG0_INTR
    accel_intr_single_level_irq_init(accelerator_ip_get_atu_mapped_cfg_address(accel_type), SDM_EXT_SDM_MSG0_INTR);

    // Clear and Enable IRQ
    // Both functions always return SUCCESS
    FPFwCoreInterruptEnableVector(IRQnum);

    // Send message to Accel emCPU to collect crash dump
    accel_intr_handle_sdm_msg_send(accel_type);
}

/**
 * @brief Clear and unmask all supported interrupts at level 1
 *
 * @details This is called when Accel emCPU has come out of reset
 * Interrupts are disabled at level 1 in ISR so need to be enabled after Accel emCPU comes out of reset
 *
 * @param[in] IRQnum : This will have IRQNum to identify if interrupt is received for CDED / SDM
 *
 * @retval void
 *
 */
static void accel_intr_clear_and_unmask_interrupts(uint32_t IRQnum)
{
    // For each individual interrupt, re initialise them
    for (eACCEL_INTR idx = ACCEL_INTR_EMCPU_WDT_ERR; idx < ACCEL_INTR_MAX; idx++)
    {
        // We are using this check to ensure that we are enabling only supported interrupts
        if (accel_intr_irq_data[idx].accel_irq_init_fn != NULL)
        {
            // Call init function for that IRQ, always returns SUCCESS
            (void)accel_intr_irq_data[idx].accel_irq_init_fn(
                accelerator_ip_get_atu_mapped_cfg_address(GET_ACCEL_TYPE_FROM_IRQ_NUMBER(IRQnum)),
                accel_intr_irq_data[idx].accel_irq_bit);
        }
    }

    // To make sure all interrupts are cleared
    cortex_m7_atomic_call_data_memory_barrier();
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

    paccel_intr_crash_dump_collection_timer_data_t paccel_intr_crash_dump_collection_timer_data =
        (paccel_intr_crash_dump_collection_timer_data_t)ctx;

    FPFW_ET_LOG(AccelIntrCrashdumpCollectTimeout,
                paccel_intr_crash_dump_collection_timer_data->IRQnum,
                paccel_intr_crash_dump_collection_timer_data->timeout_count);

    if (paccel_intr_crash_dump_collection_timer_data->timeout_count == ACCEL_INTR_CRASH_DUMP_COLLECT_FIRST_TIMEOUT)
    {
        /**
         * 1. TODO: Task 1908548: [SCP] Implementation of SDM Fatal Interrupt in SCP
         * Reset Accel emCPU and wait for emCPU to bootup
         */
        FPFW_ET_LOG(AccelIntremCPUReset, paccel_intr_crash_dump_collection_timer_data->IRQnum);

        /**
         * 2. Re-init all interrupts routed from ACCEL IP and supported in SCP in level 1 registers
         * Level 2 registers are cleared as part of Accel emCPU boot up
         * TODO: Do this when Accel emCPU reset is enabled
         * Task 1908548: [SCP] Implementation of SDM Fatal Interrupt in SCP
         */
        // accel_intr_clear_and_unmask_interrupts(paccel_intr_crash_dump_collection_timer_data->IRQnum);

        /**
         * 3. Trigger doorbell interrupt `SYS2_MSG0_INTR` again to collect ACCEL Subsystem Dump
         * 4. Create timer to wait on doorbell interrupt SDM_MSG0_INTR using fpfw_timer_create
         */
        accel_intr_request_crash_dump_collection(paccel_intr_crash_dump_collection_timer_data->IRQnum,
                                                 ACCEL_INTR_CRASH_DUMP_COLLECT_SECOND_TIMEOUT,
                                                 paccel_intr_crash_dump_collection_timer_data->is_soc_reset);
    }
    else
    {
        /** Reset timer data */
        paccel_intr_crash_dump_collection_timer_data->timeout_count = ACCEL_INTR_CRASH_DUMP_COLLECT_FIRST_TIMEOUT;
        paccel_intr_crash_dump_collection_timer_data->is_collecting_crashdump = false;

        /**
         * Request SoC reset in SCP. This should also trigger crash dump collection for ACCEL emCPU and other
         * cores. Add logic to request SoC Reset in SCP
         * TODO: Task 1908548: [SCP] Implementation of SDM Fatal Interrupt in SCP
         */
        FPFW_ET_LOG(AccelIntrSoCReset, paccel_intr_crash_dump_collection_timer_data->IRQnum);
    }
}

/*----------------------------- Global Functions ----------------------------*/

uint32_t accel_intr_crash_dump_collection_timer_init(eACCELERATOR_TYPE accel_type)
{
    fpfw_status_t status = fpfw_timer_create(&accel_intr_crash_dump_collection_timers[accel_type],
                                             FPFW_TIMER_ONESHOT,
                                             ACCEL_INTR_HANDSHAKE_TIMER_PERIOD_UNUSED_FOR_ONESHOT,
                                             (fpfw_timer_callback)accel_intr_handle_sdm_msg_recv_timeout,
                                             &accel_intr_crash_dump_collection_timer_data[accel_type]);

    if (status != FPFW_STATUS_SUCCESS)
    {
        critical_print("fpfw_timer_create failed with 0x%x\n", status);
        return ACCEL_INTR_RET_FAIL_TIMER_CREATE;
    }

    return ACCEL_INTR_RET_SUCCESS;
}

void accel_intr_handle_fatal_intr_recvd(uint32_t IRQnum)
{
    // Based on ATU MAP get sdm_ext_cfg base address
    uint32_t ext_cfg_addr = accelerator_ip_get_atu_mapped_cfg_address(GET_ACCEL_TYPE_FROM_IRQ_NUMBER(IRQnum));

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
        accel_intr_request_crash_dump_collection(IRQnum, ACCEL_INTR_CRASH_DUMP_COLLECT_FIRST_TIMEOUT, ACCEL_INTR_REQUEST_SOC_RESET);
    }
    else if (ACCEL_INTR_IS_RESET_ACCEL_EMCPU_SET(irq_status))
    {
        /**
         * 1. Send request to ACCEL emCPU to collect crash dump. This is done by raising doorbell interrupt SYS2_MSG0_INTR
         * 2. Enable IRQ from ACCEL IP
         * 3. Create timer to wait on doorbell interrupt SDM_MSG0_INTR using fpfw_timer_create
         */
        accel_intr_request_crash_dump_collection(IRQnum, ACCEL_INTR_CRASH_DUMP_COLLECT_FIRST_TIMEOUT, ACCEL_INTR_REQUEST_ACCEL_EMCPU_RESET);
    }
    else
    {
        /**
         * 1. Re-init all interrupts routed from ACCEL IP and supported in SCP in level 1 registers
         * Level 2 registers are cleared as part of Accel emCPU boot up
         */
        accel_intr_clear_and_unmask_interrupts(IRQnum);

        /**
         * 2. Clear and Enable IRQ
         */
        FPFwCoreInterruptEnableVector(IRQnum);
    }
}

void accel_intr_handle_sdm_msg_recvd(uint32_t IRQnum)
{
    eACCELERATOR_TYPE accel_type = GET_ACCEL_TYPE_FROM_IRQ_NUMBER(IRQnum);
    /**
     * Check if this interrupt was expected.
     * Else ignore the interrupt
     */
    if (accel_intr_crash_dump_collection_timer_data[accel_type].is_collecting_crashdump)
    {
        /**
         * 1. Reset the timer and inactivate it by calling fpfw_timer_reset
         * This will also reset timeout_count and is_collecting_crashdump
         */
        reset_timer(IRQnum);

        /**
         * 2. Trigger ACCEL emCPU reset
         * TODO: Task 1908548: [SCP] Implementation of SDM Fatal Interrupt in SCP
         * Reset Accel emCPU and wait for emCPU to bootup
         */
        if (accel_intr_crash_dump_collection_timer_data[accel_type].is_soc_reset)
        {
            FPFW_ET_LOG(AccelIntrSoCReset, IRQnum);
        }
        else
        {
            FPFW_ET_LOG(AccelIntremCPUReset, IRQnum);
        }

        /**
         * 2. Re-init all interrupts routed from ACCEL IP and supported in SCP in level 1 registers
         * Level 2 registers are cleared as part of Accel emCPU boot up
         */
        accel_intr_clear_and_unmask_interrupts(IRQnum);
    }
    else
    {
        /**
         * This is a spurious interrupt. Do nothing.
         */
    }
}
