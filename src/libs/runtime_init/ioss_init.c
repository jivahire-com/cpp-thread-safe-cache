//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file ioss_init.c
 * Instantiates IOSS
 */

/*------------- Includes -----------------*/
#include "ioss_ini.h" // for ioss_init

#include <DbgPrint.h>
#include <atu_api.h>
#include <bug_check.h>
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, fpfw_init_r...
#include <gpio_lib.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h> // for SOC_D0
#include <silibs_status.h>
#include <stdint.h> // for uint8_t
#include <stdio.h>  // for printf, NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(ioss, FPFW_INIT_DEPENDENCIES("vab", "mesh_stg_2", "cfg_mgr", "var_serv", "atu_svc"))
{

    if (idsw_get_die_id() == SOC_D1)
    {
        // D1 needs access to the IOSS space for GPIO interaction
        int status = gpio_set_remapped_gpio_base(GPIO_GROUP_IOSS, MSCP_ATU_AP_WINDOW_IOSS_D0_BASE_ADDR + IOSS_TOP_PL061_GPIO0_ADDRESS);
        BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, SILIBS_SUCCESS);

        FPFW_DBGPRINT_INFO("Skip IOSS init on die - 1!\n");
        goto exit;
    }
    FPFW_DBGPRINT_INFO("IOSS init\n");
    ioss_ini();
    FPFW_DBGPRINT_INFO("IOSS init complete\n");

exit:
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
