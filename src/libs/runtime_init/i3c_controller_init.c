//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file i3c_controller_init.c
 * Instantiates I3C 0 and 1 Master drivers.
 */

/*------------- Includes -----------------*/
#include <boot_status.h> // for boot_status_notify_extd
#include <bug_check.h>
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
                    FPFW_INIT_DEPENDENCIES("css_prme", "cd_init", "icc_hspmbx", "hw_ver", "icc_d2dmbx", "cfg_mgr", "gtimer_stg_2", "boot_stat"))
{
    KNG_DIE_ID die_num = idsw_get_die_id();
    DEBUG_PRINT("I3C Controller init, die_num: [%u]\n", die_num);
    boot_status_req_t boot_status_req = {0};

    boot_status_notify_extd(
        &boot_status_req,
        MSCP_BOOT_STATUS_CODE_SCP_I3C_INIT_START,
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (die_num == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY));
    int status = i3c_controller(die_num);
    if (status != 0)
    {
        DEBUG_PRINT("I3C Controller init Error status: [0x%x]\n", status);
        if (idsw_get_platform_sdv() == PLATFORM_RVP_EVT_SILICON)
        {
            BUG_ASSERT_PARAM(status == FPFW_INIT_STATUS_SUCCESS, status, die_num);
        }
    }

    boot_status_notify_extd(
        &boot_status_req,
        MSCP_BOOT_STATUS_CODE_SCP_I3C_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (die_num == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
