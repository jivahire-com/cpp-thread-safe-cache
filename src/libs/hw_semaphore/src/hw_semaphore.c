//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file hw_semaphore.c
 *  HW semaphore implementation.
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <hw_semaphore.h>
#include <idhw.h>
#include <ioss_csr_regs.h>
#include <semaphore_lib.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static KNG_STATUS translate_silibs_error(int silibs_status)
{
    switch (silibs_status)
    {
    case SILIBS_SUCCESS:
        return KNG_SUCCESS;
    case SILIBS_E_PARAM:
        return KNG_E_INVALIDARG;
    default:
        break;
    }

    return KNG_E_FAIL;
}

KNG_STATUS init_hw_semaphore()
{
    int silibs_status = SILIBS_SUCCESS;

    if (idhw_is_single_die_boot_en())
    {
        // Set semaphore group base address for DIE0 IOSS
        // Kingsgate does not support single-die DIE1 only boot.
        silibs_status = set_semaphore_group_base(SEM_GRP_ID_DIE0_IOSS,
                                                 MSCP_ATU_AP_WINDOW_IOSS_SEMAPHORE_BASE_ADDR + IOSS_CSR_SEMAPHORE_0_GET_ADDRESS);
    }
    else // Dual-die boot
    {
        // Set semaphore group base address for DIE0 IOSS
        silibs_status = set_semaphore_group_base(SEM_GRP_ID_DIE0_IOSS,
                                                 MSCP_ATU_AP_WINDOW_IOSS_SEMAPHORE_BASE_ADDR + IOSS_CSR_SEMAPHORE_0_GET_ADDRESS);

        if (silibs_status == SILIBS_SUCCESS)
        {
            // Set semaphore group base address for DIE1 IOSS
            silibs_status = set_semaphore_group_base(SEM_GRP_ID_DIE1_IOSS,
                                                     MSCP_ATU_AP_WINDOW_IOSS_SEMAPHORE1_BASE_ADDR +
                                                         IOSS_CSR_SEMAPHORE_0_GET_ADDRESS);
        }
    }

    return translate_silibs_error(silibs_status);
}