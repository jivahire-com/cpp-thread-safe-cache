/**
 * @file fuse_init.c
 * Instantiates fuse service
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <fuse_init.h>
#include <stdio.h>
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//TODO: Actual fuse service implementation is part of ADO
//https://azurecsi.visualstudio.com/Dev/_workitems/edit/908090
FPFW_INIT_COMPONENT(fuse_svc, FPFW_INIT_DEPENDENCIES("mesh", "hspmbox", "icc_mbx"))
{	
    fuse_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
