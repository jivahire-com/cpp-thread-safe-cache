//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dcs_manager.c
 * Manage data collection service interaction.
 */

/*------------- Includes -----------------*/
#include "dcs_manager_i.h"
#include "ddr_manager_i.h"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
tlm_package_t dcs_pkg_pending_buffer[MAX_PENDING_PACKAGES];
TX_QUEUE dcs_pkg_pending_queue;

/*------------- Functions ----------------*/

void dcs_manager_init(void)
{
    UINT status = tx_queue_create(&dcs_pkg_pending_queue,
                                  "dcs_pkg_pending_queue",
                                  (sizeof(tlm_package_t) + 3) / sizeof(uint32_t), // number of uint32_t elements
                                  dcs_pkg_pending_buffer,
                                  sizeof(dcs_pkg_pending_buffer));

    FpFwAssertWithArgs(status == TX_SUCCESS, status, (uintptr_t)&dcs_pkg_pending_queue, 0, 0);
}

void dcs_manager_queue_tlm_package(uintptr_t pkg_location, size_t pkg_size)
{
    tlm_package_t tlm_pkg;
    UINT tx_status;

    // TODO: dcs manager will be implemented in future task
    // //https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2023646 this temporary implementation just adds
    // the package to the pending queue, and next time around all packages are free'd
    do
    {
        tx_status = tx_queue_receive(&dcs_pkg_pending_queue, &tlm_pkg, TX_NO_WAIT);
        if (tx_status == TX_SUCCESS)
        {
            ddr_manager_deallocate_mem(&tlm_pkg.pkg_location);
        }
    } while (tx_status == TX_SUCCESS);

    tlm_pkg.pkg_location = pkg_location;
    tlm_pkg.pkg_size = pkg_size;

    tx_status = tx_queue_send(&dcs_pkg_pending_queue, &tlm_pkg, TX_NO_WAIT);
    if (tx_status != TX_SUCCESS)
    {
        // failed to pend, deallocate the memory
        ddr_manager_deallocate_mem(&tlm_pkg.pkg_location);
    }
}