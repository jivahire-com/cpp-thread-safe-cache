/**
 * @file i3c_controller_init.c
 * Instantiates I3C 0 and 1 Master drivers.
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <i3c_controller.h>
#include <idsw_kng.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(i3c_controller,
                    FPFW_INIT_DEPENDENCIES("css_prme", "cd_init", "icc_hspmbx", "hw_ver", "icc_d2dmbx", "cfg_mgr", "gtimer"))
{
    KNG_DIE_ID die_num = idsw_get_die_id();
    DEBUG_PRINT("I3C Controller init, die_num: [%u]\n", die_num);

    int status = i3c_controller(die_num);
    if (status != 0)
    {
        DEBUG_PRINT("I3C Controller init Error status: [0x%x]\n", status);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
