//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_accel.c
 * Registering accel external registers data to put in crash dump
 */

/*--------------- Includes ---------------*/
#include "crash_dump_accel.h"

#include "crash_dump_icc.h"
#include "crash_dump_memory.h"
#include "crash_dump_status.h" // for crash_dump_update_accel_state

#include <CrashDump.h>          // for FPFwCDPrintf
#include <FpFwUtils.h>          // for FPFW_MIN
#include <atu_init.h>           // for atu_svc_accel_atu_addr
#include <cded_regs_regs.h>     // for CDED_REGS_CCMP_CFG_ADDRESS
#include <cdedss_config_regs.h> // for CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS
#include <crash_dump.h>         // for crash_dump_register_address32
#include <crash_dump_events.h>  // for CRASH_DUMP_ET
#include <sdm_ext_cfg_regs.h>   // for SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS...
#include <silibs_common.h>      // for BYTES_IN_WORD32
#include <silibs_platform.h>    // for MMIO_READ32
#include <stdint.h>             // for uint32_t
#include <string.h>             // for memcpy

/**
 * All SDM external config registers are offset by 0x100000
 * in the generate header file
 */
#define ADDRESS_BLOCK_0x100000_OFFSET (SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS)

/**
 * CD magic number which indicates that CD collection is complete
 *
 */
#define CD_MAGIC_NUMBER 0xCDED5D55

#define CRASH_DUMP_MAX_RETRY_COUNT    2
#define CRASH_DUMP_MAX_RETRY_DELAY_US 2000 // 2 seconds
static uint8_t s_retry_count = 0;

/*-------------- Typedefs ----------------*/

typedef struct
{
    /* First address of 32bit registers */
    uint32_t first_reg_addr;
    /* Last address of 32bit registers  */
    uint32_t last_reg_addr;
} mmio_reg_addr_range_t;

/*-- Declarations (Statics and globals) --*/

/* Total length of external register dump is 0x36C (876) byte */
static const mmio_reg_addr_range_t accel_ext_cfg_offset[] = {
    /* emCPU config. Total length is 0x54 */
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_EMCPU_CFG_ECOREVNUM0_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_EMCPU_CFG_TSVB_LWR_ADDRESS,
    },
    /* Boot config Total length is 0x218 */
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_SDM_CTRL_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_PCIE_FUNC_CFG_ADDRESS,
    },
    /* Boot config ACE-L Total length is 0x60 */
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_ACEL_CTRL_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_ACEL1_CTRL_ADDRESS,
    },
    /* Boot config TDISP Control Register. Total length is 0x4 */
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_TDISP_CTRL_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_TDISP_CTRL_ADDRESS,
    },
    /* Mailbox 0 (HSP). Total length is 0x20 */
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_MBX0_EXT_MBX_REGS_E2I_MBX_CTRL_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_MBX0_EXT_MBX_REGS_E2I_MBX_INSTS_ADDRESS,
    },
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_MBX0_EXT_MBX_REGS_I2E_MBX_CTRL_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_MBX0_EXT_MBX_REGS_I2E_MBX_INSTS_ADDRESS,
    },
    /* External interrupt 0 (HSP). Total length is 0x14 */
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_INTR0_MSK_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_INTR0_MSG_SEND_INTR_MSK_ADDRESS,
    },
    /* Mailbox 1 (MCP). Total length is 0x20 */
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_MBX1_EXT_MBX_REGS_E2I_MBX_CTRL_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_MBX1_EXT_MBX_REGS_E2I_MBX_INSTS_ADDRESS,
    },
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_MBX1_EXT_MBX_REGS_I2E_MBX_CTRL_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_MBX1_EXT_MBX_REGS_I2E_MBX_INSTS_ADDRESS,
    },
    /* External interrupt 1 (MCP). Total length is 0x14 */
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_INTR1_MSK_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_INTR1_MSG_SEND_INTR_MSK_ADDRESS,
    },
    /* Mailbox 2 (SCP). Total length is 0x20 */
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_MBX2_EXT_MBX_REGS_E2I_MBX_CTRL_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_MBX2_EXT_MBX_REGS_E2I_MBX_INSTS_ADDRESS,
    },
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_MBX2_EXT_MBX_REGS_I2E_MBX_CTRL_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_MBX2_EXT_MBX_REGS_I2E_MBX_INSTS_ADDRESS,
    },
    /* External interrupt 2 (SCP). Total length is 0x14 */
    {
        .first_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_INTR2_MSK_ADDRESS,
        .last_reg_addr = _ADDRESSBLOCK_0X100000_MISC_SYS_EXT_INTR2_MSG_SEND_INTR_MSK_ADDRESS,
    },
};

/**
 * @brief CDED CP Config registers
 *
 * Below addresses are offset of CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS((0x200000U))
 */
static const mmio_reg_addr_range_t cded_cp_cfg_offset[] = {
    /* Version(ID) & Capabilities. Total length is 0x8 */
    {
        .first_reg_addr = CDED_CFG_REGS_CDED_VER_ADDRESS,
        .last_reg_addr = CDED_CFG_REGS_CDED_CAP_ADDRESS,
    },
    /* Number of pipelines and engines support register. */
    {
        .first_reg_addr = CDED_CFG_REGS_N_PIPES_ADDRESS,
        .last_reg_addr = CDED_CFG_REGS_PIPE_STATUS_ADDRESS + CDED_CFG_REGS_PIPE_STATUS_ARRAY_INDEX_MAX * CDED_CFG_REGS_PIPE_STATUS_ARRAY_ELEMENT_SIZE,
    },
    /* Control register. */
    {
        .first_reg_addr = CDED_CFG_REGS_RESET_ADDRESS,
        .last_reg_addr = CDED_CFG_REGS_PIPE_ENABLE_ADDRESS,
    },
    /* Debug, Interrupt, WDT and SEC registers. */
    {
        .first_reg_addr = CDED_CFG_REGS_DBG_CTRL0_ADDRESS,
        .last_reg_addr = CDED_CFG_REGS_SEC_THRESH_ADDRESS,
    },
    /* Pipeline config select registers */
    {
        .first_reg_addr = CDED_CFG_REGS_SEL_N_ADDRESS,
        .last_reg_addr = CDED_CFG_REGS_SEL_N_ADDRESS + CDED_CFG_REGS_SEL_N_ARRAY_INDEX_MAX * CDED_CFG_REGS_SEL_N_ARRAY_ELEMENT_SIZE,
    },
    /* Performance registers */
    {
        .first_reg_addr = CDED_CFG_REGS_PERF_UPTIME_ADDRESS,
        .last_reg_addr = CDED_CFG_REGS_PERF_VP_AB_ADDRESS + CDED_CFG_REGS_PERF_VP_AB_ARRAY_INDEX_MAX * CDED_CFG_REGS_PERF_VP_AB_ARRAY_ELEMENT_SIZE,
    },
};

/**
 * @brief CDED CP Compression engine registers
 *
 * Below addresses are offset of CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS + CDED_CFG_REGS_SIZE
 */
static const mmio_reg_addr_range_t cded_cp_ccmp_offset[] = {
    /* Version(ID) & Capabilities. */
    {
        .first_reg_addr = CCMP_CFG_REGS_CCMP_VER_ADDRESS,
        .last_reg_addr = CCMP_CFG_REGS_CCMP_CAP_ADDRESS,
    },
    /* Status */
    {
        .first_reg_addr = CCMP_CFG_REGS_CCMP_STAT_ADDRESS,
        .last_reg_addr = CCMP_CFG_REGS_CCMP_STAT_ADDRESS,
    },
    /* Control, Error, History Buffers, Hash Table, Huffman parameter,  */
    {
        .first_reg_addr = CCMP_CFG_REGS_CCMP_CTRL_ADDRESS,
        .last_reg_addr = CCMP_CFG_REGS_HUF_DISABLE_MODES_ADDRESS,
    },
    /* Debug and Interrupt */
    {
        .first_reg_addr = CCMP_CFG_REGS_DBG_CTRL0_LZ77_ADDRESS,
        .last_reg_addr = CCMP_CFG_REGS_ISR_FORCE_2_ADDRESS,
    },
};

/**
 * @brief CDED CP Decompression engine registers
 *
 * Below addresses are offset of
 * CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS + CDED_CFG_REGS_SIZE + CDED_REGS_CCMP_CFG_ARRAY_ELEMENT_SIZE * CDED_REGS_CCMP_CFG_ARRAY_COUNT
 */
static const mmio_reg_addr_range_t cded_cp_dcmp_offset[] = {
    /* Version(ID) & Capabilities. */
    {
        .first_reg_addr = DCMP_CFG_REGS_DCMP_VER_ADDRESS,
        .last_reg_addr = DCMP_CFG_REGS_DCMP_ERR_ADDRESS,
    },
    /* Debug and Interrupt */
    {
        .first_reg_addr = DCMP_CFG_REGS_DBG_CTRL0_2_ADDRESS,
        .last_reg_addr = DCMP_CFG_REGS_ISR_FORCE_3_ADDRESS,
    },
};

/**
 * @brief CDED CP AES engine registers
 *
 * Below addresses are offset of
 * CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS +
 * CDED_CFG_REGS_SIZE +
 * CDED_REGS_CCMP_CFG_ARRAY_ELEMENT_SIZE * CDED_REGS_CCMP_CFG_ARRAY_COUNT
 * CDED_REGS_DCMP_PL_ARRAY_ELEMENT_SIZE * CDED_REGS_DCMP_PL_ARRAY_COUNT
 */
static const mmio_reg_addr_range_t cded_cp_aes_cfg_offset[] = {
    /* Version(ID) & Capabilities. */
    {
        .first_reg_addr = AES_PL_CFG_REGS_AES_PL_CFG_GRP_AES_VER_ADDRESS,
        .last_reg_addr = AES_PL_CFG_REGS_AES_PL_CFG_GRP_AES_CAP_ADDRESS,
    },
    /* Status, Error */
    {
        .first_reg_addr = AES_PL_CFG_REGS_AES_PL_CFG_GRP_AES_STAT_ADDRESS,
        .last_reg_addr = AES_PL_CFG_REGS_AES_PL_CFG_GRP_AES_ERR_ADDRESS,
    },
    /* Control */
    {
        .first_reg_addr = AES_PL_CFG_REGS_AES_PL_CFG_GRP_AES_CTRL_ADDRESS,
        .last_reg_addr = AES_PL_CFG_REGS_AES_PL_CFG_GRP_AES_CTRL_ADDRESS,
    },
};

/**
 * @brief CDED CP AES engine registers
 *
 * Below addresses are offset of
 * CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS +
 * CDED_CFG_REGS_SIZE +
 * CDED_REGS_CCMP_CFG_ARRAY_ELEMENT_SIZE * CDED_REGS_CCMP_CFG_ARRAY_COUNT
 * CDED_REGS_DCMP_PL_ARRAY_ELEMENT_SIZE * CDED_REGS_DCMP_PL_ARRAY_COUNT
 */
static const mmio_reg_addr_range_t cded_cp_aes_offset[] = {
    /* Debug and Interrupt */
    {
        .first_reg_addr = AES_PL_CFG_REGS_DBG_CTRL0_3_ADDRESS,
        .last_reg_addr = AES_PL_CFG_REGS_ISR_FORCE_4_ADDRESS,
    },
};

/*--------- Private Function ----------*/

/**
 * @brief Registers an array of MMIO registers with CD framework
 * @param type_ctx Pointer to crash dump type context
 * @param first_reg_addr Address of the first register in the array of registers
 * @param last_reg_addr  Address of the last register in the array of registers
 * @param offset An offset that needs to be added to all the register addresses
 * above to access this registers.
 * @return Total count of registered registers.
 */
static uint32_t accel_register_mmio_register(crash_dump_type_context_t* type_ctx,
                                             uint32_t first_reg_addr,
                                             uint32_t last_reg_addr,
                                             uint32_t offset)
{
    uint32_t mmio_reg = offset + first_reg_addr;
    uint32_t reg_count = ((last_reg_addr - first_reg_addr) / BYTES_IN_WORD32) + 1;

    CdRegisterMMIORegisterSet(&type_ctx->crash_dump_ctx, mmio_reg, reg_count, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    return reg_count;
}

/**
 * @brief Registers a array of array of MMIO registers with CD framework.
 * Lets call this structure as set of MMIO registers
 * @param type_ctx Pointer to crash dump type context
 * @param mmio_reg_set Pointer to set of array of MMIO registers
 * @param mmio_reg_set_count Number of array of MMIO registers
 * @param offset An offset that needs to be added to all the register addresses
 * described within above structure to access this registers.
 * @return Total count of registered registers.
 */
static uint32_t accel_register_mmio_register_set(crash_dump_type_context_t* type_ctx,
                                                 const mmio_reg_addr_range_t* mmio_reg_set,
                                                 uint32_t mmio_reg_set_count,
                                                 uint32_t offset)
{
    uint32_t reg_count = 0;

    for (uint32_t i = 0; i < mmio_reg_set_count; i++)
    {
        reg_count +=
            accel_register_mmio_register(type_ctx, mmio_reg_set[i].first_reg_addr, mmio_reg_set[i].last_reg_addr, offset);
    }

    return reg_count;
}

/**
 * @brief Registers a array of set of MMIO registers with CD framework.
 * Lets call this structure as group of MMIO registers
 *
 * Group of MMIO registers is the same register set repeated again and again.
 * Example of group of MMIO register are CDED engines. CDED core has
 * multiple compression engines, each engine will have its own set of
 * registers and this set of registers will be same for all compression
 * engines. So all the registers of a compression engine will form a register
 * group.
 *
 * @param type_ctx Pointer to crash dump type context
 * @param mmio_reg_group_count Number of register sets
 * @param mmio_reg_group_size Size of register sets. Only represents the address
 * space of the register set.
 * @param mmio_reg_set Pointer to set of array of MMIO registers
 * @param mmio_reg_set_count Number of array of MMIO registers
 * @param offset An offset that needs to be added to all the register addresses
 * described within above structure to access this registers.
 * @return Total count of registered registers.
 */
static uint32_t accel_register_mmio_register_group(crash_dump_type_context_t* type_ctx,
                                                   uint32_t mmio_reg_group_count,
                                                   uint32_t mmio_reg_group_size,
                                                   const mmio_reg_addr_range_t* mmio_reg_set,
                                                   uint32_t mmio_reg_arr_count,
                                                   uint32_t offset)
{
    uint32_t group_idx;
    uint32_t reg_count = 0;

    for (group_idx = 0; group_idx < mmio_reg_group_count; group_idx++)
    {
        reg_count += accel_register_mmio_register_set(type_ctx, mmio_reg_set, mmio_reg_arr_count, offset);
        offset += mmio_reg_group_size;
    }

    return reg_count;
}

static uint32_t crash_dump_register_sdm_ext_mmio(crash_dump_type_context_t* type_ctx)
{
    uint32_t atu_map_addr;
    uint32_t reg_count;

    atu_map_addr = atu_svc_accel_atu_addr(ACCEL_ID_SDM);

    /* Register CDED config registers */
    reg_count = accel_register_mmio_register_set(type_ctx,
                                                 accel_ext_cfg_offset,
                                                 FPFW_ARRAY_SIZE(accel_ext_cfg_offset),
                                                 atu_map_addr + ADDRESS_BLOCK_0x100000_OFFSET);

    return reg_count;
}

static uint32_t crash_dump_register_cded_ext_mmio(crash_dump_type_context_t* type_ctx)
{
    uint32_t atu_map_addr;
    uint32_t offset = 0;
    uint32_t reg_count = 0;

    atu_map_addr = atu_svc_accel_atu_addr(ACCEL_ID_CDED);

    /* Register CDED config registers */
    reg_count += accel_register_mmio_register_set(type_ctx,
                                                  accel_ext_cfg_offset,
                                                  FPFW_ARRAY_SIZE(accel_ext_cfg_offset),
                                                  atu_map_addr + ADDRESS_BLOCK_0x100000_OFFSET);

    /* Register CDED CP config registers */
    offset += CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    reg_count +=
        accel_register_mmio_register_set(type_ctx, cded_cp_cfg_offset, FPFW_ARRAY_SIZE(cded_cp_cfg_offset), atu_map_addr + offset);

    /* Register CDED CP CCMP engine registers */
    offset += CDED_REGS_CDED_CFG_REGS_SIZE;
    reg_count += accel_register_mmio_register_group(type_ctx,
                                                    CDED_REGS_CCMP_CFG_REGS_ARRAY_COUNT,
                                                    CDED_REGS_CCMP_CFG_REGS_ARRAY_ELEMENT_SIZE,
                                                    cded_cp_ccmp_offset,
                                                    FPFW_ARRAY_SIZE(cded_cp_ccmp_offset),
                                                    atu_map_addr + offset);

    /* Register CDED CP DCMP engine registers */
    offset += CDED_REGS_CCMP_CFG_REGS_SIZE;
    reg_count += accel_register_mmio_register_group(type_ctx,
                                                    CDED_REGS_DCMP_CFG_REGS_ARRAY_COUNT,
                                                    CDED_REGS_DCMP_CFG_REGS_ARRAY_ELEMENT_SIZE,
                                                    cded_cp_dcmp_offset,
                                                    FPFW_ARRAY_SIZE(cded_cp_dcmp_offset),
                                                    atu_map_addr + offset);

    /* Register CDED CP AES engine config registers */
    offset += CDED_REGS_DCMP_CFG_REGS_SIZE;
    reg_count += accel_register_mmio_register_group(type_ctx,
                                                    AES_PL_CFG_REGS_AES_PL_CFG_REGS_AES_PL_CFG_GRP_ARRAY_COUNT,
                                                    AES_PL_CFG_REGS_AES_PL_CFG_REGS_AES_PL_CFG_GRP_ARRAY_ELEMENT_SIZE,
                                                    cded_cp_aes_cfg_offset,
                                                    FPFW_ARRAY_SIZE(cded_cp_aes_cfg_offset),
                                                    atu_map_addr + offset);

    /* Register CDED CP AES engine registers */
    reg_count +=
        accel_register_mmio_register_set(type_ctx, cded_cp_aes_offset, FPFW_ARRAY_SIZE(cded_cp_aes_offset), atu_map_addr + offset);

    return reg_count;
}

static bool crash_dump_wait_for_accel_cd_complete(ACCEL_ID accel_type)
{
    /**
     * CD collection of SDM and CDED happens in parallel. Wait only for the first
     * accel core to complete the crash dump collection. The later accel core
     * should have completed the crash dump collection by the time we reach here.
     */
    while (crash_dump_is_accel_cd_complete(accel_type) == false)
    {
        if (s_retry_count >= CRASH_DUMP_MAX_RETRY_COUNT)
        {
            return false;
        }

        s_retry_count++;
        // Allow some time for the accel core to complete the crash dump collection
        SLEEP_US(CRASH_DUMP_MAX_RETRY_DELAY_US);
    }

    return true;
}

static void copy_cd_file_dtcm_to_ddr(crash_dump_context_t* ctx, ACCEL_ID accel_type)
{
    uint32_t cd_ddr_addr;
    uint32_t accel_dtcm_addr;
    uint32_t cd_file_size = ctx->accel_cd_ctx[accel_type].cd_file_size;
    uint32_t cd_file_off = ctx->accel_cd_ctx[accel_type].cd_file_offset;

    if (ctx->type_ctx[CRASH_DUMP_TYPE_FULL] == NULL)
    {
        // no full dump context.
        CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_ACCEL_INVALID_ADDRESS);
        goto cd_transfer_failed;
    }

    if (cd_file_size == 0 || cd_file_off + cd_file_size > SDM_EXT_CFG_EMCPU_TCM_DTCM_SIZE)
    {
        // Invalid DTCM address or size.
        CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_ACCEL_INVALID_SIZE);
        goto cd_transfer_failed;
    }

    if (!crash_dump_wait_for_accel_cd_complete(accel_type))
    {
        // Invalid MAGIC number address or CD collection is not complete for given accel core
        CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_ACCEL_CD_INCOMPLETE);
        goto cd_transfer_failed;
    }

    if (accel_type == ACCEL_ID_SDM)
    {
        cd_ddr_addr = CRASH_DUMP_FULL_SDM_ADDR;
        cd_file_size = FPFW_MIN(CRASH_DUMP_FULL_SDM_SIZE, cd_file_size);
    }
    else
    {
        cd_ddr_addr = CRASH_DUMP_FULL_CDED_ADDR;
        cd_file_size = FPFW_MIN(CRASH_DUMP_FULL_CDED_SIZE, cd_file_size);
    }

    accel_dtcm_addr = atu_svc_accel_atu_addr(accel_type) + SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS;
    memcpy((void*)cd_ddr_addr, (void*)(accel_dtcm_addr + cd_file_off), cd_file_size);
    crash_dump_update_accel_state(accel_type, CRASH_DUMP_STATE_COMPLETED);
    return;

cd_transfer_failed:
    crash_dump_update_accel_state(accel_type, CRASH_DUMP_STATE_NOT_AVAILABLE);
    CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_ACCEL_TRANSFER_FAILED);
}

/*------------- Public Functions ----------------*/
/**
 * @brief Register SDM cores MMIO registers
 *
 */
void crash_dump_register_accel_ext_mmio(ACCEL_ID accel_type)
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (ctx->type_ctx[CRASH_DUMP_TYPE_FULL] == NULL || accel_type >= NUM_VALID_ACCEL_ID)
    {
        // No full dump context or invalid accel type.
        return;
    }

    if (accel_type == ACCEL_ID_SDM)
    {
        // Register SDM core registers only for full dump.
        crash_dump_register_sdm_ext_mmio(ctx->type_ctx[CRASH_DUMP_TYPE_FULL]);
    }
    else
    {
        // Register CDED core registers only for full dump.
        crash_dump_register_cded_ext_mmio(ctx->type_ctx[CRASH_DUMP_TYPE_FULL]);
    }
}

/**
 * @brief Copies accel device crashdump from accel DTCM to DDR
 *
 */
void crash_dump_copy_accel_cd_file(void* ctx)
{
    crash_dump_context_t* cd_ctx = crash_dump_context();
    ACCEL_ID accel_type = (ACCEL_ID)ctx;

    if (cd_ctx == NULL || accel_type >= NUM_VALID_ACCEL_ID)
    {
        CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_ACCEL_INVALID_ADDRESS);
        return;
    }

    /* Copy accel crashdump file from accel DTCM to DDR */
    copy_cd_file_dtcm_to_ddr(cd_ctx, accel_type);
}

uint32_t crash_dump_transfer_accel_cd_to_BMC(ACCEL_ID accel_type)
{
    crash_dump_copy_accel_cd_file((void*)accel_type);

    return crash_dump_request_transfer_dump();
}

bool crash_dump_is_accel_cd_complete(ACCEL_ID accel_type)
{
    crash_dump_context_t* cd_ctx = crash_dump_context();
    uint32_t* cd_magic_nr;

    if (cd_ctx == NULL)
    {
        CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_ACCEL_INVALID_ADDRESS);
        return false;
    }

    cd_magic_nr = (void*)cd_ctx->accel_cd_ctx[accel_type].cd_magic_nr_offset;
    if ((uint32_t)cd_magic_nr + sizeof(*cd_magic_nr) > SDM_EXT_CFG_EMCPU_TCM_DTCM_SIZE)
    {
        // Invalid magic number address.
        CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_ACCEL_INVALID_SIZE);
        return false;
    }

    // Compute the ATU mapped address of the magic number
    uint32_t accel_dtcm_addr = atu_svc_accel_atu_addr(accel_type) + SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS;
    cd_magic_nr = (void*)cd_magic_nr + accel_dtcm_addr;

    if (MMIO_READ32(cd_magic_nr) != CD_MAGIC_NUMBER)
    {
        // SDM/CDED busy
        CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_ACCEL_MAGIC_NUMBER_UNMATCHED);
        return false;
    }

    return true;
}
