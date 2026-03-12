//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file hw_version_scp_init.c
 * Setups up System ID components for the SCP
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT...
#include <idhw.h>      // for idhw_get_die_id, idhw_get_platform_...
#include <idsw.h>      // for idsw_set_cpu_type, idsw_set_die_id
#include <idsw_kng.h>
#include <silibs_scp_top_regs.h> // for SCP_TOP_SID_ADDRESS
#include <stddef.h>              // for NULL
#include <stdint.h>              // for uint32_t, uintptr_t
#include <utils.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

PLACED_CODE FPFW_INIT_COMPONENT(hw_ver, FPFW_INIT_DEPENDENCIES("mpu"))
{

    /* Set System ID Base Address*/
    idhw_set_sid_base((uintptr_t)SCP_TOP_SID_ADDRESS);

    /* Set CPU type to SCP */
    idsw_set_cpu_type(CPU_SCP);

    /* Fetch SoC ID from SID Regs*/
    uint32_t hw_soc_id = idhw_get_soc_id();
    FPFW_UNUSED(hw_soc_id);

    /* Fetch Die ID from SID Regs and set SW Die ID for firmware */
    idsw_set_die_id(idhw_get_die_id());

    /* Get platform ID from SID Regs */
    KNG_PLAT_ID hw_platform_id = idhw_get_platform_id_from_hw();

    /* Set SW platform ID for firmware */
    idsw_set_platform_sdv(hw_platform_id);

    /* SVT, Single Die Boot Enable Regs skipped */

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
