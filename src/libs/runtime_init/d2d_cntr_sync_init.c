/**
 * @file d2d_cntr_sync_init.c
 * Synchronizes counter of d1 with d0 for uniform timestamp across dies
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <d2d_cntr_sync.h>
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <stddef.h> // for NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(d2d_cntr_sync, FPFW_INIT_DEPENDENCIES("hw_ver", "debug_print", "gtimer_stg_2", "cfg_mgr"))
{
    //! d2d sync counters are not supported on SVP currently
    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        KNG_DIE_ID die_num = idsw_get_die_id();
        KNG_CPU_TYPE cpu_type = idsw_get_cpu_type();
        d2d_cntr_sync_init(die_num, cpu_type);
        FPFW_DBGPRINT_INFO("D2D Sync Counter Done, die_num: [%u]\n", die_num);
    }
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
