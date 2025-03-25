/**
 * @file nvic_init.c
 * Initialize the NVIC
 */

/*------------- Includes -----------------*/
#include "accel_intr_virt_irq.h"
#include "accelip_id.h"

#include <FpFwUtils.h> // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <fpfw_init.h>
#include <interrupts.h>
#include <nvic.h>
#include <virt_irq.h>

/*-- Symbolic Constant Macros (defines) --*/

#define TOTAL_VIRTUAL_IRQ_COUNT (NUM_VALID_ACCEL_ID)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

virt_irq_plat_cb_t* mscp_virt_irq_plat_info[TOTAL_VIRTUAL_IRQ_COUNT];

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(virt_irq, FPFW_INIT_DEPENDENCIES("nvic"))
{
    virt_irq_plat_cb_t* accel_virt_irq_plat_info = NULL;
    uint32_t accel_virt_irq_cnt = accel_intr_get_virt_irq_plat_info(&accel_virt_irq_plat_info);

    for (uint8_t i = 0; i < accel_virt_irq_cnt; i++)
    {
        mscp_virt_irq_plat_info[i] = &accel_virt_irq_plat_info[i];
    }

    virt_irq_init(mscp_virt_irq_plat_info, FPFW_ARRAY_SIZE(mscp_virt_irq_plat_info));
    accel_intr_virt_irq_register_isr(HW_INT_SDM_COMB_INT, HW_INT_CDED_COMB_INT);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
