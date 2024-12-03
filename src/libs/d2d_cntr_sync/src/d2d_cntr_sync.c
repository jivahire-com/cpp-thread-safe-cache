//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file d2d_cntr_sync.c
 *  Sets up d2d sync for all subsystems for each die
 */

/*------------- Includes -----------------*/
#include <d2d_cntr_sync.h> // for d2d_counter_sync_init
#include <d2d_counter_struct_defaults.h> // for DEFAULT_INSTANCE_D2D_COUNTER_SYNC_D0_CONFIG_T, DEFAULT_INSTANCE_D2D_COUNTER_SYNC_D1_CONFIG_T
#include <d2d_counter_sync.h> // for d2d_counter_sync_init
#include <stddef.h>           // for NULL
#include <stdint.h>           // for uint32_t

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void d2d_cntr_sync_init(KNG_DIE_ID die_num)
{
    //! Initialize config params for D2D counter sync logic for SCPs
    if (die_num == DIE_0)
    {
        DEFAULT_INSTANCE_D2D_COUNTER_SYNC_D0_CONFIG_T(d0_config);
        d2d_counter_sync_configure_d0_params(&d0_config);
    }
    else
    {
        DEFAULT_INSTANCE_D2D_COUNTER_SYNC_D1_CONFIG_T(d1_config);
        d2d_counter_sync_configure_d1_params(&d1_config);

        //! @todo Adjust system counter skew on die 1 for uniform counter value across dies
        //! Since counter can start on die 0 before die 1
        //! Ensure die 0 has configured & enabled it's counter syn before die 1 tried to trigger immediate
        //! sync Add check to verify sync success, silibs to provide API for this.
        //! https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/821622/
    }

    //! Enable periodic Sync of counters post d2dss_sequence, it has 2 pre requisites,
    //! 1. the D2D link is up and we are sure the SCPs on both dies are in sync
    //! 2. gtimers are initialized
    d2d_counter_sync_enable((DIE_INSTANCE)die_num);
}