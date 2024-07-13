/**
 * @file scp_icc_mhu_trans_init.c
 * Instantiates baremetal, polling based, ICC MHU - for SCP
 */

/*------------- Includes -----------------*/

#include <FpFwAssert.h>
#include <fpfw_init.h>               // for fpfw_init_get_handle, FPFW_INIT...
#include <fpfw_status.h>             // for fpfw_status_t
#include <icc_mhu_trans_prim.h>      // for icc mhu transport APIs and definitions
#include <stdint.h>                  // for uint32_t
#include <stdio.h>

/*-------------- Macros ------------------*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(icc_mhu_trans, FPFW_INIT_DEPENDENCIES("dfwk"))
{
    // Initialize just the primitives for now
    icc_mhu_trans_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

