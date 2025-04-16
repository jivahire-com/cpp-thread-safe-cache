/**
 * @file accelerator_ip_mcp_init.c
 * Initialize the Accelerators on MCP core
 */

/*------------- Includes -----------------*/
#include <accel_intr.h>         // for accel_intr_set_irq_num_for_accel
#include <accelerator_ip.h>     // for accelerators_init, accel_is_isolation_enabled
#include <accelip_id.h>         // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <atu_init.h>           // for accel_atu_config
#include <fpfw_init.h>          // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMP...
#include <interrupts.h>         // for HW_INT_SDM_INTR1_MCP_COMB_INT
#include <stdio.h>              // for NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(accel, FPFW_INIT_DEPENDENCIES("hw_ver", "accel_intr_clnt", "nvic", "accel_atu", "virt_irq", "cfg_mgr"))
{
    // Update accel irq numbers used in irq init
    accel_intr_set_irq_num_for_accel(ACCEL_ID_SDM, HW_INT_SDM_COMB_INT);
    accel_intr_set_irq_num_for_accel(ACCEL_ID_CDED, HW_INT_CDED_COMB_INT);

    // Initialize the Accelerators
    mcp_accelerators_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(accel_atu, FPFW_INIT_DEPENDENCIES("hw_ver", "atu_svc", "nvic", "cfg_mgr"))
{
    // Initialize the Accelerators
    for (ACCEL_ID accel_type = ACCEL_ID_SDM; accel_type < NUM_VALID_ACCEL_ID; accel_type++)
    {
        if (!accel_is_isolation_enabled(accel_type))
        {
            accel_atu_config(accel_type);
        }
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
