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
#include <fpfw_cfg_mgr.h>
#include <idsw.h>   // for idsw_get_platform_sdv, idsw...
#include <string.h> // for memcpy, strlen
#ifdef WORKAROUND_ACCEL_WARM_RESET
    // ToDo: Remove this flag and the code guarded by it after SDM/CDED warm reset implemented.
    //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/2496178/
    #include <system_info.h> // for system_info_is_warm_start
#endif

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
bool accel_is_isolation_enabled(ACCEL_ID accel_type)
{
    bool is_enabled = false;
    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();

#ifdef WORKAROUND_ACCEL_WARM_RESET
    // ToDo: Remove this flag and the code guarded by it after SDM/CDED warm reset implemented.
    //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/2496178/
    if (system_info_is_warm_start())
    {
        // If warm start, return true for isolation enabled until accelerators are ready for warm start.
        return true;
    }
#endif

    // Read Knob value for SDM/CDED Isolation as per Die id
    switch (accel_type)
    {
    case ACCEL_ID_SDM:
        is_enabled = config_get_sdm_isolation_enable().isolation_enable[current_die_instance];
        break;
    case ACCEL_ID_CDED:
        is_enabled = config_get_cded_isolation_enable().isolation_enable[current_die_instance];
        break;
    default:
        is_enabled = false;
    }

    return is_enabled;
}

int32_t scp_accelerators_isolation_control(void)
{
    // TODO: read the knob (and fuse if implemented).  For now we are hardcoding to de-isolate
    // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2259268
    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();
    int32_t ret = ACCEL_RET_SUCCESS;
    uint32_t accel_ctxt_size = 0;

    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    FPFW_RUNTIME_ASSERT(p_ss_ctxt != NULL);

    // Init all available Accelerator instances
    for (uint32_t index = 0; index < accel_ctxt_size; index++)
    {
        // TODO (ADO 1728772) : init any particular accelerator instance only if that is enabled in fuses
        if (p_ss_ctxt[index].accelip_metadata.die_instance == current_die_instance)
        {
            atu_map_entry_t atu_map_entry;
            memcpy((void*)&atu_map_entry, (void*)p_ss_ctxt[index].p_accelip_atu_map, sizeof(atu_map_entry_t));

            ret = atu_map(ATU_ID_MSCP, &atu_map_entry);
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);

            ret = accelip_ss_enable_ip_isolation(
                atu_map_entry.mscp_start_address,
                p_ss_ctxt[index].accelip_metadata.accel_type,
                accel_is_isolation_enabled(get_accelip_type(p_ss_ctxt[index].accelip_metadata.accel_type)));
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);

            ret = atu_unmap(ATU_ID_MSCP, &atu_map_entry);
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);
        }
    }

    /* Returning ACCEL_RET_SUCCESS since any condition where ret != ACCEL_RET_SUCCESS results in an assert.
    Change if needed. */
    return ACCEL_RET_SUCCESS;
}