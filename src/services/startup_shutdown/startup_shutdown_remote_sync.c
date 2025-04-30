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
#include <icc_platform_defines.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <mscp_exp_rmss_memory_map.h>
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
            mscp_exp_spi_sync_point_t d2d_sos_sync_point;
            d2d_sos_sync_point.local_write_addr = SCP_EXP_D2D_SYNC_APCORE_BASE;
            d2d_sos_sync_point.remote_write_addr = SCP_EXP_D2D_SYNC_APCORE_BASE + sizeof(uint32_t);
            d2d_sos_sync_point.value = (RMSS_D2D_SPI_SYNC_ENUM_START | current_boot_stage.stage);

            status = mscp_exp_spi_synchronize_dies(d2d_sos_sync_point, idsw_get_die_id());
        }
    }

    return status == SILIBS_SUCCESS;
}
