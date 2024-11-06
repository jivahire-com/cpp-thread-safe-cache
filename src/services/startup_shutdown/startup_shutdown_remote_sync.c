//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_remote_sync.c
 * Implements the startup or shutdown thread for SSI notifications
 */

/*------------- Includes -----------------*/
#include "startup_shutdown_i.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <mscp_exp_spi_synchronize_dies.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
bool wait_for_remote_die_boot_stage(startup_shutdown_boot_stage_t current_boot_stage)
{
    int status = SILIBS_SUCCESS;

    if (idhw_is_single_die_boot_en() == false)
    {
        if (current_boot_stage.remote_die_sync_required)
        {
            //! Synchronize with the remote die at this point
            //! D0 writes data to remote SRAM at offset 0x0 & polls for D1 to write to it's own local SRAM at offset 0x4
            //! D1 writes to local SRAM at offset 0x4 & polls for D0 to write to it's local SRAM at offset 0x0
            d2d_sync_point.value = current_boot_stage.stage;
            status = mscp_exp_spi_synchronize_dies(d2d_sync_point, idsw_get_die_id());
        }
    }

    return status == SILIBS_SUCCESS;
}
