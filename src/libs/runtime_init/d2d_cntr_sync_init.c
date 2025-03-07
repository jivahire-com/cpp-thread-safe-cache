/**
 * @file d2d_cntr_sync_init.c
 * Synchronizes counter of d1 with d0 for uniform timestamp across dies
 */

/*------------- Includes -----------------*/
#include <d2d_cntr_sync.h>
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(d2d_cntr_sync, FPFW_INIT_DEPENDENCIES("hw_ver", "std_io", "mesh", "gtimer"))
{
    //! d2d sync counters are not supported on SVP currently
    if (!(IS_PLATFORM_SVP()))
    {
        KNG_DIE_ID die_num = idsw_get_die_id();
        d2d_cntr_sync_init(die_num);
        printf("d2d_cntr_sync done, die[%u]\n", die_num);
    }
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
