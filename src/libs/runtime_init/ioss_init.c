/**
 * @file ioss_init.c
 * Instantiates IOSS
 */

/*------------- Includes -----------------*/
#include "ioss_init.h" // for ioss_init

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
    // TODO: ADO 1826681 Re-enable on FPGA once the vab sequence library has been integrated
    if ((idsw_get_platform_sdv() == PLATFORM_FPGA) || (idsw_get_platform_sdv() == PLATFORM_FPGA_LARGE))
    {
        printf("%s: skip IOSS init on FPGA for now!\n", __func__);
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    uint8_t die_num = (uint8_t)idhw_get_die_id();
    if (die_num == SOC_D0)
    {
        printf("IOSS init, die_num: [%u]\n", die_num);
        ioss_init(die_num);
    }
    else
    {
        printf("IOSS not used on die: %u... Skipping IOSS init...\n", die_num);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}