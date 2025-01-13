//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_isolation.c
 * This file provides interface to configure accelerator IP(s) isolation
 */

/*-------------------------------- Features ---------------------------------*/

/*-------------------------------- Includes ---------------------------------*/
#include "accelerator_ip.h"

#include <FpFwAssert.h>          // for FPFW_RUNTIME_ASSERT
#include <accelerator_ip_priv.h> // for get_accelerator_ctxt
#include <atu_lib.h>             // for atu_map, atu_unmap, atu_map...
#include <idsw.h>                // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <kng_soc_constants.h> // for DIE_INSTANCE
#include <silibs_platform.h>   // for debug_print, MMIO_UPDATE32
#include <silibs_status.h>     // for SILIBS_SUCCESS
#include <stdint.h>            // for int32_t, uintptr_t, uint32_t
#include <string.h>            // for memcpy, strlen

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

int32_t scp_accelerators_isolation_control(void)
{
    // TODO: read the knob (and fuse if implemented).  For now we are hardcoding to de-isolate
    // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2259268
    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();
    int32_t ret = ACCEL_RET_SUCCESS;
    uint32_t accel_ctxt_size = 0;

    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    FPFW_RUNTIME_ASSERT(p_ss_ctxt != NULL);

    debug_print("Number of Accelerator instances present: %d\n", (int)accel_ctxt_size);

    // Init all available Accelerator instances
    for (uint32_t index = 0; index < accel_ctxt_size; index++)
    {
        // TODO (ADO 1728772) : init any particular accelerator instance only if that is enabled in fuses
        if (p_ss_ctxt[index].accelip_metadata.die_instance == current_die_instance)
        {
            debug_print("accel lib: Initializing for die_id = %d, accel_type = %d, accel_instance = %d\n",
                        p_ss_ctxt[index].accelip_metadata.die_instance,
                        p_ss_ctxt[index].accelip_metadata.accel_type,
                        p_ss_ctxt[index].accelip_metadata.accel_instance);

            atu_map_entry_t atu_map_entry;
            memcpy((void*)&atu_map_entry, (void*)p_ss_ctxt[index].p_accelip_atu_map, sizeof(atu_map_entry_t));

            ret = atu_map(ATU_ID_MSCP, &atu_map_entry);
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);

            ret = accelip_ss_enable_ip_isolation(atu_map_entry.mscp_start_address,
                                                 p_ss_ctxt[index].accelip_metadata.accel_type,
                                                 p_ss_ctxt[index].p_init_params->accelip_ss_cfg->isolation_enable);
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);

            ret = atu_unmap(ATU_ID_MSCP, &atu_map_entry);
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);
        }
    }

    /* Returning ACCEL_RET_SUCCESS since any condition where ret != ACCEL_RET_SUCCESS results in an assert.
    Change if needed. */
    return ACCEL_RET_SUCCESS;
}