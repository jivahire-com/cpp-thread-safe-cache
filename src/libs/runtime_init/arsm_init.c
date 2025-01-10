//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file arsm_init.c
 * This file contains the initialization of the ARSM.
 */

/*------------- Includes -----------------*/

#include <atu_api.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <string.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(arsm, FPFW_INIT_DEPENDENCIES("atu_svc", "hw_ver", "sysinfo"))
{
    // Only clear the ARSM on a cold boot
    if (!system_info_is_warm_start())
    {
        uintptr_t arsm_base = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR;
        if (idsw_get_die_id() != DIE_0)
        {
            arsm_base = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR;
        }

        // The ARSM size is the same on either die.
        memset((void*)arsm_base, 0, MSCP_ATU_AP_WINDOW_ARSM_DIE_0_SIZE);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
