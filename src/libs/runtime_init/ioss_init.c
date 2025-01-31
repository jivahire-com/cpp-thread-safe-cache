/**
 * @file ioss_init.c
 * Instantiates IOSS
 */

/*------------- Includes -----------------*/
#include "ioss_ini.h" // for ioss_init

#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, fpfw_init_r...
#include <idhw.h>      // for idhw_get_die_id
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h> // for SOC_D0
#include <stdint.h>            // for uint8_t
#include <stdio.h>             // for printf, NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(ioss, FPFW_INIT_DEPENDENCIES("vab", "tower_cfg"))
{

    if (idsw_get_die_id() == SOC_D1)
    {
        printf("Skip IOSS init on die - 1!\n");
        goto exit;
    }
    printf("IOSS init\n");
    ioss_ini();
    printf("IOSS init complete\n");

exit:
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}