/**
 * @file scp_mpu_init.c
 * Instantiates MPU for the SCP
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <mpu.h>
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// TODO: Still TBD if Normal WB RWA is valid given Boot RAM cannot do 8 or 16 bit writes.
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
            .RBAR = ARM_MPU_RBAR(1, SCP_TOP_SCP_INST_RAM_ADDRESS),  // NOLINT
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
            .RBAR = ARM_MPU_RBAR(2, SCP_TOP_SCP_DATA_RAM_ADDRESS),  // NOLINT
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
         * MPU Region 3~6 - MSCP_EXP SRAM (RAM0 + (13 * 64kb of RAM1))
         *                  Outer and inner Write-Back. Write and read allocate
         *                  Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(3, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 NON_SHAREABLE,
                                 CACHEABLE,
                                 BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_1MB),
        },
        {
            .RBAR = ARM_MPU_RBAR(4, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 NON_SHAREABLE,
                                 CACHEABLE,
                                 BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_512KB),
        },
        {
            .RBAR = ARM_MPU_RBAR(5, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS + SCP_EXP_TOP_RAM1_SIZE/2),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 NON_SHAREABLE,
                                 CACHEABLE,
                                 BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_256KB),
        },
        {
            .RBAR = ARM_MPU_RBAR(6, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS + (3 * SCP_EXP_TOP_RAM1_SIZE / 4)),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 NON_SHAREABLE,
                                 CACHEABLE,
                                 BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_64KB),
        },
        /**
         * MPU Region 7~9 - MSCP_EXP SRAM (last 192kb of RAM1))
         *                  Strongly Ordered
         *                  Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(7, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS + (13 * SCP_EXP_TOP_RAM1_SIZE / 16)),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_0,
                                 SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_64KB),
        },
        {
            .RBAR = ARM_MPU_RBAR(8, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS + (14 * SCP_EXP_TOP_RAM1_SIZE / 16)),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_0,
                                 SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_64KB),
        },
        {
            .RBAR = ARM_MPU_RBAR(9, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS + (15 * SCP_EXP_TOP_RAM1_SIZE / 16)),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_0,
                                 SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_64KB),
        },
        /**
         * MPU Region 10 - MSCP_EXP SCF RAM
         *                 Strongly Ordered
         *                
         */
        {
            .RBAR = ARM_MPU_RBAR(10, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS), // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_0,
                                 SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_64KB),
        },
        /**
         * MPU Region 11 - System memory PPB (Cortex-M7 registers)
         *                Strongly Ordered
         *                Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(11, SCP_TOP_CORTEX_M7_ADDRESS), // NOLINT
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

    // Disable Dynamic read allocate mode
    SCnSCB->ACTLR |= SCnSCB_ACTLR_DISRAMODE_Msk;

    uint8_t region_count = sizeof(regions) / sizeof(regions[0]);
    mpu_init(regions, region_count);

    // Enable I caches,  ensures that any stale instructions are not used.
    SCB_InvalidateICache();
    SCB_EnableICache();

    // Enable D caches, ensures any stale data is not used.
    SCB_InvalidateDCache();
    SCB_EnableDCache();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
