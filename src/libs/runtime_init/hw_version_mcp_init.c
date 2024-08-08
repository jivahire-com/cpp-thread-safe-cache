/**
 * @file hw_version_mcp_init.c
 * Setups up System ID components for the MCP
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT...
#include <idsw.h>      // for idsw_set_cpu_type, idsw_set_die_id
#include <idsw_kng.h>
#include <stddef.h>              // for NULL
#include <stdint.h>              // for uint32_t, uintptr_t
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(hw_ver, FPFW_INIT_DEPENDENCIES("mpu"))
{

    /* Set CPU type to MCP */
    idsw_set_cpu_type(CPU_MCP);

    /**
     * @note 
     * Any aspects involving SID/IDHW cannot be called here for MCP because it requires the ATU init.
     * SID registers are in AP address space and needs ATU map/unmap to access it.
     * which we will need to do after MCP is bootloaded by SCP (post mesh init).
     * Hence we do not have the ability to query for platform type on MCP. 
     */

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
