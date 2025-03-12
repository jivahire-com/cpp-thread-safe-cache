/**
 * @file mcp_mpu_init.c
 * Instantiates MPU for the MCP
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <fpfw_init.h>
#include <mpu.h>
#include <silibs_mcp_exp_top_regs.h>
#include <silibs_mcp_top_regs.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(mpu, FPFW_INIT_NULL_NODE)
{
    // clang-format off
    // Setup regions
    //  - All regions must have a start address that is aligned to the size of the region. 
    const ARM_MPU_Region_t regions[] = {
        /**
         * MPU Region 0 - Background   0x00000000 - 0xFFFFFFFF
         *                Shared Device, XN=1
         *                Protects the entire address space from I-side and D-side speculative
         *                accesses if not overridden by the higher number MPU regions below.
         *                Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(0, 0x00000000),    // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_0,
                                 SHAREABLE,
                                 NON_CACHEABLE,
                                 BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_4GB),
        },
       /**
        * MPU Region 1 - ITCM
        *                Normal Non-Cacheable, Non-Shareable 
        *                M7 won't cache TCM regardless of MPU configuration
        *                see Cortex-M7 TRM section 5.7.1 TCM attributes and permissions
        *                Privileged R/W (so bootRAM relocation can write) FIXME: Make R/O after bootRAM relocation?
        */
        {
            .RBAR = ARM_MPU_RBAR(1, MCP_TOP_MCP_INST_RAM_ADDRESS),  // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 NON_SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_512KB),
        },
        /**
         * MPU Region 2 - DTCM
         *                Normal Non-Cacheable, Non-Shareable, XN=1
         *                M7 won't cache TCM regardless of MPU configuration
         *                (see Cortex-M7 TRM section 5.7.1 TCM attributes and permissions)
         *                Priviledged R/W     
         */
        {
            .RBAR = ARM_MPU_RBAR(2, MCP_TOP_MCP_DATA_RAM_ADDRESS),  // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 NON_SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_512KB),
        },
        /**
         * MPU Region 3 - MSCP_EXP SRAM (RAM0 + RAM1)
         *                Normal Noncacheable, Non-bufferable
         *                Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(3, MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 NON_SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_2MB),
        },
        /**
         *  @todo 2457199 https://azurecsi.visualstudio.com/Dev/_workitems/edit/2457199
         * Add MPU settings for rmss rodata region.
         * For MCP starting address = 0x013BF490, Size = 64 KB
         * 
         * MPU Region 4 - MSCP_EXP SRAM RODATA 
         *                Cacheable
         *                Priviledged R Only
         */
        /**
         * MPU Region 5 - MSCP_EXP SCF RAM
         *                Normal Noncacheable
         *                Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(4, MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_RAM_ADDRESS), // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_64KB),
        },
        /**
         * MPU Region 6 - DDR Used for In-Band Telemetry
         *                Normal Noncacheable
         *                Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(5, MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR), // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_128MB),
        },
        /**
         * MPU Region 6 - System memory PPB (Cortex-M7 registers)
         *                Strongly Ordered
         *                Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(6, MCP_TOP_CORTEX_M7_ADDRESS), // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_0,
                                 NON_SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_1MB),
        },
    };
    // clang-format on

    uint8_t region_count = sizeof(regions) / sizeof(regions[0]);

    mpu_init(regions, region_count);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
