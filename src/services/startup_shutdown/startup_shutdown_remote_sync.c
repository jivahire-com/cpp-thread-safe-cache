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
#include <arm_intrinsic.h>
#include <atu_api.h>
#include <cmsis_m7.h>
#include <fpfw_init.h>
#include <hw_semaphore.h>
#include <icc_platform_defines.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_scp_tfa_shared.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_exp_spi_synchronize_dies.h>
#include <semaphore_lib.h>

/*-- Symbolic Constant Macros (defines) --*/
#define ARSM_SYNC_SEMAPHORE_KEY(die_number) \
    ((((die_number) & 0xFF) << 24) | (0x41443244 & 0xFFFFFF)) // 'A''D''2''D'

#ifndef D1_ARSM_MSCP_D2D_SYNC_POINT_BASE
    #define D1_ARSM_MSCP_D2D_SYNC_POINT_SIZE (8)
    #define D1_ARSM_MSCP_D2D_SYNC_POINT_BASE (D1_ARSM_MSCP_D2D_POWER_DATA_END + 1)
    #define D1_ARSM_MSCP_D2D_SYNC_POINT_END \
        (D1_ARSM_MSCP_D2D_SYNC_POINT_BASE + D1_ARSM_MSCP_D2D_SYNC_POINT_SIZE - 1)
#endif

/*------------- Typedefs -----------------*/
typedef mscp_exp_spi_sync_point_t mscp_arsm_sync_point_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
int arsm_synchronize_dies(mscp_exp_spi_sync_point_t sp, uint32_t local_die_id);

/*------------- Functions ----------------*/
bool wait_for_remote_die_boot_stage(startup_shutdown_boot_stage_t current_boot_stage)
{
    int status = SILIBS_SUCCESS;
    uintptr_t sync_point_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D1_ARSM_MSCP_D2D_SYNC_POINT_BASE);

    if (idhw_is_single_die_boot_en() == false)
    {
        if (current_boot_stage.remote_die_sync_required)
        {
            mscp_arsm_sync_point_t d2d_sos_sync_point;
            d2d_sos_sync_point.local_write_addr = sync_point_base;
            d2d_sos_sync_point.remote_write_addr = sync_point_base + sizeof(uint32_t);
            d2d_sos_sync_point.value = (RMSS_D2D_SPI_SYNC_ENUM_START | current_boot_stage.stage);

            status = arsm_synchronize_dies(d2d_sos_sync_point, idsw_get_die_id());
        }
    }

    return status == SILIBS_SUCCESS;
}

int arsm_synchronize_dies(mscp_exp_spi_sync_point_t sp, uint32_t local_die_id)
{
    uintptr_t my_slot_addr;
    uintptr_t peer_slot_addr;

    // Pick which slot we own based on our die ID
    if (local_die_id == DIE_0)
    {
        my_slot_addr = sp.local_write_addr;
        peer_slot_addr = sp.remote_write_addr;
    }
    else
    {
        my_slot_addr = sp.remote_write_addr;
        peer_slot_addr = sp.local_write_addr;
    }

    uint32_t semaphore_key = ARSM_SYNC_SEMAPHORE_KEY(local_die_id);

    // REQUIREMENT: my_stage must be monotonically increasing across calls.
    const uint32_t my_stage = sp.value;

    // Publish our stage
    wait_for_semaphore(ARSM_D2DSYNC_SEMAPHORE_ID, semaphore_key);
    mmio_write32((volatile uint32_t*)my_slot_addr, my_stage);
    __DSB();
    release_semaphore(ARSM_D2DSYNC_SEMAPHORE_ID);

    // Wait for the peer to reach *at least* this stage.
    // This avoids the race where the peer advances to N+1 before we observe N.
    while (1)
    {
        wait_for_semaphore(ARSM_D2DSYNC_SEMAPHORE_ID, semaphore_key);
        uint32_t peer_val = mmio_read32((volatile uint32_t*)peer_slot_addr);
        __DSB();
        release_semaphore(ARSM_D2DSYNC_SEMAPHORE_ID);

        if (peer_val >= my_stage)
        {
            break; // Rendezvous complete for this stage
        }

        tx_thread_sleep(1);
    }

    return SILIBS_SUCCESS;
}