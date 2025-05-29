/**
 * @file ioss_init.c
 * Instantiates IOSS
 */

/*------------- Includes -----------------*/
#include "ioss_ini.h" // for ioss_init

#include <DbgPrint.h>
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, fpfw_init_r...
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
FPFW_INIT_COMPONENT(ioss, FPFW_INIT_DEPENDENCIES("vab", "mesh_stg_2", "cfg_mgr", "var_serv"))
{

    if (idsw_get_die_id() == SOC_D1)
    {
        FPFW_DBGPRINT_INFO("Skip IOSS init on die - 1!\n");
        goto exit;
    }
    FPFW_DBGPRINT_INFO("IOSS init\n");
    ioss_ini();
    FPFW_DBGPRINT_INFO("IOSS init complete\n");

exit:
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
