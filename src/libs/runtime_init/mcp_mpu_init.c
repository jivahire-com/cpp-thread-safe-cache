/**
 * @file mcp_mpu_init.c
 * Instantiates MPU for the MCP
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
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
         *                Normal Write-Back, Read and Write Allocate
         *                Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(3, MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 NON_SHAREABLE,
                                 CACHEABLE,
                                 BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_2MB),
        },
        /**
         * MPU Region 4 - MSCP_EXP SCF RAM
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
         * MPU Region 5 - System memory PPB (Cortex-M7 registers)
         *                Strongly Ordered
         *                Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(5, MCP_TOP_CORTEX_M7_ADDRESS), // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_0,
                                 NON_SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_1MB),
        }
    };
    // clang-format on

    uint8_t region_count = sizeof(regions) / sizeof(regions[0]);

    mpu_init(regions, region_count);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
