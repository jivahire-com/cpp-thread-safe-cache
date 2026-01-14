//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file mcp_mpu_init.c
 * Instantiates MPU for the MCP
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <fpfw_init.h>
#include <kng_scp_tfa_shared.h>
#include <mpu.h>
#include <silibs_mcp_exp_top_regs.h>
#include <silibs_mcp_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define IS_ALIGNED_4K(addr) (((uintptr_t)(addr) & 0xFFF) == 0)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// NOTE:  these asserts are for memory regions 13 and 14 below.

static_assert(IS_ALIGNED_4K(MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(D0_ARSM_SDS_AP_BASE)),
              "mpu region not 4k aligned");

static_assert(((D0_ARSM_SDS_AP_BASE + 0x2000 - 1) < D0_ARSM_TFA_RP_DATA_BASE), "overlap TFA");

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
         * MPU Region 3~6 - MSCP_EXP SRAM (RAM0 + (13 * 64kb of RAM1))
         *                  Outer and inner Write-Back. Write and read allocate
         *                  Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(3, MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 SHAREABLE,
                                 CACHEABLE,
                                 BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_1MB),
        },
        {
            .RBAR = ARM_MPU_RBAR(4, MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 SHAREABLE,
                                 CACHEABLE,
                                 BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_512KB),
        },
        {
            .RBAR = ARM_MPU_RBAR(5, MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS + MCP_EXP_TOP_RAM1_SIZE/2),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 SHAREABLE,
                                 CACHEABLE,
                                 BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_256KB),
        },
        {
            .RBAR = ARM_MPU_RBAR(6, MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS + (3 * MCP_EXP_TOP_RAM1_SIZE / 4)),    // NOLINT
            .RASR = ARM_MPU_RASR(ENABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 SHAREABLE,
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
            .RBAR = ARM_MPU_RBAR(7, MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS + (13 * MCP_EXP_TOP_RAM1_SIZE / 16)),    // NOLINT
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
            .RBAR = ARM_MPU_RBAR(8, MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS + (14 * MCP_EXP_TOP_RAM1_SIZE / 16)),    // NOLINT
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
            .RBAR = ARM_MPU_RBAR(9, MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS + (15 * MCP_EXP_TOP_RAM1_SIZE / 16)),    // NOLINT
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
         *                Normal Noncacheable
         *                Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(10, MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_RAM_ADDRESS), // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 SHAREABLE,
                                 CACHEABLE,
                                 BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_64KB),
        },
        /**
         * MPU Region 11 - DDR Used for In-Band Telemetry
         *                Normal Noncacheable
         *                Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(11, MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR), // NOLINT
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
         * MPU Region 12 - System memory PPB (Cortex-M7 registers)
         *                Strongly Ordered
         *                Priviledged R/W
        */
        {
            .RBAR = ARM_MPU_RBAR(12, MCP_TOP_CORTEX_M7_ADDRESS), // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_0,
                                 NON_SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_1MB),
        },
        /**
         * MPU Region 13 - SDS, the first few ICC buffers. Allows unaligned access
         *                There is 8K of ARSM used by MSCP starting with the SDS address.
         *                However the start address is not 8K aligned which needs to match the
         *                region size, so two 4k mpu regions are used. This is the first 4k
         *                Note static asserts above.
         *                Normal Noncacheable
         *                Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(13, MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR +
                                   ARSM_GET_REGION_OFFSET(D0_ARSM_SDS_AP_BASE)), // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_4KB),
        },
         /**
         * MPU Region 14 - Remaining ICC buffers and Power Telemetry die to die exchange. Allows unaligned access
         *                There is 8K of ARSM used by MSCP starting with the SDS address.
         *                However the start address is not 8K aligned which needs to match the
         *                region size, so two 4k mpu regions are used. This is the second 4k.
         *                Note static asserts above.
         *                Normal Noncacheable
         *                Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(14, MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR +
                                   ARSM_GET_REGION_OFFSET(D0_ARSM_SDS_AP_BASE + 0x1000)), // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_4KB),
        },
        /**
         * MPU Region 15 - DDR Payloads used for ICC MHU
         *                 Normal Noncacheable
         *                 Priviledged R/W
         */
        {
            .RBAR = ARM_MPU_RBAR(15, MSCP_ATU_AP_WINDOW_ICC_MHU_PAYLOAD_BASE_ADDR), // NOLINT
            .RASR = ARM_MPU_RASR(DISABLE_EXEC,
                                 ARM_MPU_AP_PRIV,
                                 TYPE_EXT_1,
                                 SHAREABLE,
                                 NON_CACHEABLE,
                                 NON_BUFFERABLE,
                                 DISABLE_SUBREGION,
                                 ARM_MPU_REGION_SIZE_16KB),
        },
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
