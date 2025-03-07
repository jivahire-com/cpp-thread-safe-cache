/**
 * @file variable_services_init.c
 * Instantiates variable services
 */

/*------------- Includes -----------------*/
#include <fpfw_icc_base.h> // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_init.h>
#include <idhw.h>
#include <stdint.h>
#include <stdio.h>
#include <variable_services.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(var_serv, FPFW_INIT_DEPENDENCIES("icc_hspmbx", "sysinfo"))
{
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");
    variable_service_init(icc_ctx);

    printf("var_serv init done, die[%u]\n", die_num);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
