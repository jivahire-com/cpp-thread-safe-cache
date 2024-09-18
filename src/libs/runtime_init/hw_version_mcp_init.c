/**
 * @file hw_version_mcp_init.c
 * Setups up System ID components for the MCP
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>
#include <atu_lib.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h>
#include <silibs_ap_top_regs.h>
#include <silibs_mcp_top_regs.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/


/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(hw_ver, FPFW_INIT_DEPENDENCIES("mpu"))
{

    /** 
     * The MCP doesn't have access to the SID registers directly, instead it has to get them from the AP Address Space.
     *
     * 1. Initialize the ATU with a single map entry for the SID Registers
     * 2. Read the SID Registers and cache the results into idsw
     * 
     * This only works if the MCP has been released from reset post the SCP Configuring the Mesh, which aligns with the
     * POR boot sequence. If the MCP boots before then the below code will cause a Bus Fault.
     *
     * The ATU Service will own the static maps used during nominal runtime, overwriting our map here.
     */
    
    atu_map_entry_t sid_registers_atu_map[] = {
        {
            .ap_base_address = AP_TOP_D0_SYS_ID_REGS_ADDRESS,
            .mscp_start_address = MCP_TOP_MCP_TO_AP_ADMAP_MEM_ADDRESS,
            .mscp_end_address = ALIGN_UP(MCP_TOP_MCP_TO_AP_ADMAP_MEM_ADDRESS + AP_TOP_D0_SYS_ID_REGS_SIZE, ATU_PAGE_SIZE) - 1,
            .attribute = {ATU_BUS_ATTR_NS},
        },
        {0},
    };

    // Init the lib to fill in ATU CSR regs
    FPFW_RUNTIME_ASSERT(SILIBS_SUCCESS == atu_init(ATU_ID_MSCP, sid_registers_atu_map, 1));

    /* Set System ID Register base Address */
    idhw_set_sid_base((uintptr_t)MCP_TOP_MCP_TO_AP_ADMAP_MEM_ADDRESS);

    // Cache from idhw to idsw
    idsw_set_cpu_type(CPU_MCP);
    idsw_set_die_id(idhw_get_die_id());
    idsw_set_platform_sdv(idhw_get_platform_id_from_hw());

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
