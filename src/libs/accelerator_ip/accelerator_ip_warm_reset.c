//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_warm_reset.c
 * This file provides the implementation of the warm reset sequence for the Accelerator IP(s).
 */

/*-------------------------------- Features ---------------------------------*/

/*-------------------------------- Includes ---------------------------------*/

#include "accelerator_ip.h" // for accel_core_warm_reset

#include <DbgPrint.h>                    // for FPFW_DBGPRINT_INFO
#include <DfwkPtrTypes.h>                // for PDFWK_ASYNC_REQUEST_HEADER
#include <FPFwInterrupts.h>              // for FPFwCoreInterruptEnableVector
#include <FpFwUtils.h>                   // for FPFW_UNUSED
#include <_addressblock_0x100000_regs.h> // for ptr__addressblock_0x100000_reg
#include <accel_intr.h>           // for accel_intr_emcpu_wdt_control, ACCEL_INTR_ENABLE_ACCEL_EMCPU_WDT
#include <accelerator_ip_priv.h>  // for ACCEL_NAME_LEN
#include <accelip_id.h>           // for NUM_VALID_ACCEL_ID, ACCEL_ID_SDM
#include <atu_init.h>             // for atu_svc_accel_atu_addr
#include <bug_check.h>            // for BUG_ASSERT
#include <cdedss_config_regs.h>   // for CDEDSS_CONFIG_SDM_EXT_CFG_ADDRESS
#include <fpfw_init.h>            // for fpfw_init_get_handle
#include <idsw.h>                 // for idsw_get_die_id
#include <sdm_ext_cfg_regs.h>     // for SDM_EXT_CFG_EMCPU_TCM_ITCM_ADDRESS
#include <sdm_init.h>             // for sdm_init_itcm_enable
#include <sdmss_config_regs.h>    // for SDMSS_CONFIG_SDM_EXT_CFG_ADDRESS
#include <silibs_platform.h>      // for MMIO_READ32, debug_print
#include <startup_shutdown.h>     // for sos_start_phase
#include <startup_shutdown_ssi.h> // for COLD_BOOT, STARTUP_PHASE_MSCP_ASYNC
#include <stdbool.h>              // for false, true
#include <stdint.h>               // for uintptr_t, uint32_t
#include <string.h>               // for memcpy, strlen

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

static uint32_t get_accel_ext_cfg_offset_addr(ACCEL_ID accel_type)
{
    switch (accel_type)
    {
    case ACCEL_ID_SDM:
        return SDMSS_CONFIG_SDM_EXT_CFG_ADDRESS;
    case ACCEL_ID_CDED:
        return CDEDSS_CONFIG_SDM_EXT_CFG_ADDRESS;
    default:
        BUG_ASSERT(false);
        break;
    }

    return SDMSS_CONFIG_SDM_EXT_CFG_ADDRESS; // To satisfy x86 compiler
}

// API toggles ITCM enable -> 0 -> 1
static void sdm_init_itcm_toggle(const uintptr_t ext_cfg_addr)
{
    volatile ptr__addressblock_0x100000_reg reg_addr =
        (ptr__addressblock_0x100000_reg)(ext_cfg_addr + SDM_EXT_CFG_EMCPU_TCM_ITCM_SIZE + SDM_EXT_CFG_EMCPU_TCM_DTCM_SIZE);

    sdm_init_itcm_enable(ext_cfg_addr, false);
    do
    {
        // Wait for ITCM EN to become 0
    } while ((MMIO_READ32(&reg_addr->emcpu_cfg_tcm_ctrl) >> _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MSB) &
             _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MASK);

    sdm_init_itcm_enable(ext_cfg_addr, true);
    do
    {
        // Wait for ITCM EN to become 1
    } while (!((MMIO_READ32(&reg_addr->emcpu_cfg_tcm_ctrl) >> _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MSB) &
               _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MASK));
}

/**
 * @brief This function is called after the warm reset sequence is completed.
 * This function re-enables the emCPU watchdog timer and the interrupt for the accelerator.
 * It undoes all the operations done before warm reset.
 * @param request The request header containing the async request data.
 * @param ctx The context pointer passed into the callback as argument.
 */
static void accel_post_warm_reset_cb(PDFWK_ASYNC_REQUEST_HEADER request, void* ctx)
{
    FPFW_UNUSED(request);

    ACCEL_ID accel_type = (ACCEL_ID)ctx;
    uint32_t IRQnum = accel_intr_get_irq_num_from_accel_type(accel_type);
    uint32_t ext_cfg_addr = atu_svc_accel_atu_addr(accel_type);

    accel_intr_emcpu_wdt_control(ext_cfg_addr, ACCEL_INTR_ENABLE_ACCEL_EMCPU_WDT);
    FPFwCoreInterruptEnableVector(IRQnum);
}

static void accel_emcpu_reset_sequence(uintptr_t sdm_ext_cfg_base)
{
    /**
     * Sequence to reset Embed CPU without letting it execute
     * CPUWAIT      = 1
     * FENCE_EN     = 1
     * NSYSRESET    = 0
     * ITCM_EN      = 0
     * DELAY
     * ITCM_EN      = 1
     * FENCE_EN     = 0
     * NSYSRESET    = 1
     */

    sdm_init_enable_cpuwait(sdm_ext_cfg_base);
    sdm_init_enable_fence(sdm_ext_cfg_base, true);
    sdm_init_assert_nsysreset(sdm_ext_cfg_base);

    // Currently only directly disabling/enabling ITCM EN
    sdm_init_itcm_toggle(sdm_ext_cfg_base);

    sdm_init_enable_fence(sdm_ext_cfg_base, false);
    sdm_init_deassert_nsysreset(sdm_ext_cfg_base);
}

static void accel_warm_boot_sequence(ACCEL_ID accel_type, uintptr_t sdm_ext_cfg_base)
{
    static startup_start_phase_request_t startup_accel_warm_reset_request[NUM_VALID_ACCEL_ID];
    ssi_startup_stage_t startup_stage;

    accel_emcpu_reset_sequence(sdm_ext_cfg_base);

    startup_stage = accel_type == ACCEL_ID_SDM ? STARTUP_WARM_BOOT_SDM_ASYNC : STARTUP_WARM_BOOT_CDED_ASYNC;

    /* Re-init the entire interrupt tree to start from clean state once accel core is up */
    accel_intr_scp_init(accel_type, sdm_ext_cfg_base, E_ACCEL_INTR_INIT_FULL_INTR_TREE);

    DfwkAsyncRequestInitialize((void*)&startup_accel_warm_reset_request[accel_type].header.async,
                               sizeof(startup_accel_warm_reset_request[accel_type]));

    sos_start_phase(fpfw_init_get_handle("sos_int"),
                    &startup_accel_warm_reset_request[accel_type],
                    WARM_BOOT_ACCEL,
                    startup_stage,
                    accel_post_warm_reset_cb,
                    (void*)accel_type);
}

/*----------------------------- Global Functions ----------------------------*/

void accel_disable_cpu_wait(ACCEL_ID accel_type)
{
    uint32_t ext_cfg_offset_addr = get_accel_ext_cfg_offset_addr(accel_type);

    sdm_init_disable_cpu_wait((atu_svc_accel_atu_addr(accel_type) + ext_cfg_offset_addr));
}

void accel_core_warm_reset(ACCEL_ID accel_type)
{
    BUG_ASSERT(accel_type < NUM_VALID_ACCEL_ID);

    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();
    uint32_t ext_cfg_offset_addr = get_accel_ext_cfg_offset_addr(accel_type);

    FPFW_DBGPRINT_INFO("%s: Accel type %d, Die %d\n", __func__, accel_type, current_die_instance);

    accel_warm_boot_sequence(accel_type, (ext_cfg_offset_addr + atu_svc_accel_atu_addr(accel_type)));
}

void accel_core_suspend(ACCEL_ID accel_type)
{
    BUG_ASSERT_PARAM(accel_type < NUM_VALID_ACCEL_ID, accel_type, 0);

    uint32_t emcpu_addr = get_accel_ext_cfg_offset_addr(accel_type) + atu_svc_accel_atu_addr(accel_type);
    FPFW_DBGPRINT_INFO("%s: Accel type %d CORE_SUSP\n", __func__, accel_type);

    accel_emcpu_reset_sequence(emcpu_addr);
}
