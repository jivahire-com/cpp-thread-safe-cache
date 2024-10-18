/**
 * @file variable_services_cli_init.c
 * Instantiates variable services cli commands
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <mscp_exp_rmss_memory_map.h>
#include <stdint.h>
#include <stdio.h>
#include <variable_services.h>
#include <variable_services_cli.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(var_serv_cli, FPFW_INIT_DEPENDENCIES("var_serv"))
{
    var_service_shared_mem_t mem_ctx = {
        .payload_base = (uintptr_t)SCP_EXP_SCP_TEST_VARIABLE_SERVICE_PAYLOAD_BASE,
        .max_payload_size = SCP_EXP_SCP_TEST_VARIABLE_SERVICE_PAYLOAD_SIZE,
    };
    variable_services_cli_init(&mem_ctx);
    printf("Variable Service Cli Init Done\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
