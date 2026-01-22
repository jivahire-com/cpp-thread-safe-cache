//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_mcp.c
 * This file provides APIs specific to MCP to engage with Accelerator devices
 */

/* TODO 2228988: Create separate Lib for SCP & MCP specific functionalities */

/*-------------------------------- Features ---------------------------------*/

/*-------------------------------- Includes ---------------------------------*/

#include "accelerator_ip.h"
#include "accelerator_ip_events.h"
#include "accelerator_ip_priv.h"

#include <DbgPrint.h>      // for FPFW_DBGPRINT_INFO
#include <accel_intr.h>    // for accel_intr_mcp_init
#include <accelip_id.h>    // NUM_VALID_ACCEL_ID, ACCEL_ID_SDM, ACCEL_ID_CDED
#include <atu_init.h>      // for atu_svc_accel_atu_addr
#include <atu_lib.h>       // for atu_map, atu_unmap, atu_map...
#include <bug_check.h>     // for BUG_ASSERT_PARAM
#include <idsw.h>          // for idsw_get_die_id
#include <idsw_kng.h>      // for IS_PLATFORM_FPGA
#include <silibs_status.h> // for SILIBS_SUCCESS
#include <stdint.h>        // for int32_t, uintptr_t, uint32_t
#include <stdio.h>         // for printf, NULL
#include <string.h>        // for memcpy

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

static int32_t init_accelerator(subsystem_ctxt_t* p_ss_ctxt)
{
    int32_t ret = ACCEL_RET_SUCCESS;
    ACCEL_ID accel_type = get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type);

    if (accel_type == NUM_VALID_ACCEL_ID)
    {
        return ACCEL_RET_FAIL_INVALID_PARAMS;
    }

    ret = accel_mcp_intr_init(accel_type);
    if (ret != ACCEL_INTR_RET_SUCCESS)
    {
        FPFW_ET_LOG(AccelIPMCPError, accel_type, ret, __LINE__);
        return ACCEL_RET_FAIL_INTR_INIT;
    }

    return ACCEL_RET_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/

int32_t mcp_accelerators_init(void)
{
    idsw_die_id_t current_die_instance = idsw_get_die_id();
    int ret = ACCEL_RET_SUCCESS;
    uint32_t accel_ctxt_size = 0;

    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    BUG_ASSERT_PARAM(p_ss_ctxt != NULL, p_ss_ctxt, 0);

    // Init all available Accelerator instances
    for (uint32_t index = 0; index < accel_ctxt_size; index++)
    {
        // TODO (ADO 1728772) : init any particular accelerator instance only if that is enabled in fuse
        if (p_ss_ctxt[index].accelip_metadata.die_instance == current_die_instance)
        {
            if (!accel_is_isolation_enabled(get_accelip_type(p_ss_ctxt[index].accelip_metadata.accel_type)))
            {
                ret = init_accelerator(&p_ss_ctxt[index]);
                FPFW_DBGPRINT_INFO("accel lib: Die %d type %d instance %d stat %d\n",
                                   p_ss_ctxt[index].accelip_metadata.die_instance,
                                   p_ss_ctxt[index].accelip_metadata.accel_type,
                                   p_ss_ctxt[index].accelip_metadata.accel_instance,
                                   ret);
                BUG_ASSERT_PARAM(ret == ACCEL_RET_SUCCESS, ret, 0);
            }
            else
            {
                FPFW_DBGPRINT_INFO("Accel type %d on Die %d has been isolated\n",
                                   p_ss_ctxt[index].accelip_metadata.accel_type,
                                   current_die_instance);
            }
        }
    }

    return ret;
}
