//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file variable_services_cli_init.c
 * Instantiates variable services cli commands
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <atu_api.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <mscp_exp_rmss_memory_map.h>
#include <stdint.h>
#include <stdio.h>
#include <variable_services.h>
#include <variable_services_cli.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @note 
 * Var Serv Cli only supported for SCP 0/1
 * For DIE1, DDR subsystem must be initialized prior to atu service
 */
FPFW_INIT_COMPONENT(var_serv_cli, FPFW_INIT_DEPENDENCIES("var_serv", "ddr", "atu_svc", "sysinfo"))
{
    var_service_shared_mem_t mem_ctx = {0};
    KNG_DIE_ID die_num = idhw_get_die_id();
    if (die_num == DIE_0)
    {
        //! Payload resides in RMSS Ram space
        mem_ctx.payload_base = (uintptr_t)SCP_EXP_SCP_TEST_VARIABLE_SERVICE_PAYLOAD_BASE;
        mem_ctx.max_payload_size = SCP_EXP_SCP_TEST_VARIABLE_SERVICE_PAYLOAD_SIZE;
    }
    else
    {
        //! Payload resides in DDR space (Die 1 SCP can't access RMSS Ram)
        mem_ctx.payload_base = (uintptr_t)MSCP_ATU_AP_WINDOW_VAR_SVC_TEST_PAYLOAD_BASE;
        mem_ctx.max_payload_size = MSCP_ATU_AP_WINDOW_VAR_SVC_TEST_PAYLOAD_BASE_SIZE;
    }
    
    variable_services_cli_init(&mem_ctx);
    FPFW_DBGPRINT_INFO("var_serv_cli Init Done, die[%u] MemBase[0x%x] Size[0x%x]\n", die_num, mem_ctx.payload_base, mem_ctx.max_payload_size);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}