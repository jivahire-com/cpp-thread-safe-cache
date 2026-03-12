//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file nvic_init.c
 * Initialize the NVIC
 */

/*------------- Includes -----------------*/
#include "accel_intr_virt_irq.h"
#include "accelip_id.h"

#include <FpFwUtils.h> // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <boot_status.h>
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <nvic.h>
#include <utils.h>
#include <virt_irq.h>

/*-- Symbolic Constant Macros (defines) --*/

#define TOTAL_VIRTUAL_IRQ_COUNT (NUM_VALID_ACCEL_ID)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

virt_irq_plat_cb_t* mscp_virt_irq_plat_info[TOTAL_VIRTUAL_IRQ_COUNT];

/*------------- Functions ----------------*/

PLACED_CODE FPFW_INIT_COMPONENT(virt_irq, FPFW_INIT_DEPENDENCIES("nvic", "cfg_mgr", "boot_stat"))
{
    virt_irq_plat_cb_t* accel_virt_irq_plat_info = NULL;
    uint32_t accel_virt_irq_cnt = accel_intr_get_virt_irq_plat_info(&accel_virt_irq_plat_info);

    for (uint8_t i = 0; i < accel_virt_irq_cnt; i++)
    {
        mscp_virt_irq_plat_info[i] = &accel_virt_irq_plat_info[i];
    }

    virt_irq_init(mscp_virt_irq_plat_info, FPFW_ARRAY_SIZE(mscp_virt_irq_plat_info));
    accel_intr_virt_irq_register_isr(HW_INT_SDM_COMB_INT, HW_INT_CDED_COMB_INT);

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_VIRT_IRQ_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_VIRT_IRQ_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
