//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_overrides.c
 * File containing all crash dump override functions for the framework
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>              // for FPFW_DEBUG_ASSERT
#include <silibs_scp_exp_top_regs.h> // for SCP_EXP_TOP_SCF_RAM_ADDRESS, SCP_EXP_TOP_SCF_RAM_SIZE
#include <silibs_scp_top_regs.h> // for SCP_TOP_SCP_EXP_ADDRESS, SCP_TOP_SCP_INST_RAM_ADDRESS, SCP_TOP_SCP_INST_RAM_SIZE
#include <stdbool.h>             // for bool
#include <stdint.h>              // for uint32_t, uint64_t

/*-- Symbolic Constant Macros (defines) --*/
#define CD_DEFAULT_MEM_POOL_SIZE 256

#define SCP_SCF_RAM_ADDRESS  (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS)
#define SCP_EXP_RAM0_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
#define SCP_EXP_RAM1_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint8_t s_cd_block_pool_memory[CD_DEFAULT_MEM_POOL_SIZE] = {};

/*------------- Functions ----------------*/

/*------- Memory Pool Overrides -------*/
/**
 * @brief Get the crash dump mem pool object
 *        ToDo: Will re-visit once SCP crash dump memory pool region is defined.
 *
 * @param mem_pool_addr
 * @param size
 */
void get_crash_dump_mem_pool(uint64_t* mem_pool_addr, uint32_t* size)
{
    *mem_pool_addr = (uint64_t)((uintptr_t)s_cd_block_pool_memory);
    *size = CD_DEFAULT_MEM_POOL_SIZE;
}

/*------- Dump File Overrides -------*/
bool in_memory(uintptr_t start_addr, uintptr_t end_addr)
{

    FPFW_DEBUG_ASSERT(start_addr <= end_addr);

    // limit to RAM regions to avoid holes in register space
    if ((end_addr < SCP_TOP_SCP_INST_RAM_ADDRESS + SCP_TOP_SCP_INST_RAM_SIZE) ||
        (SCP_SCF_RAM_ADDRESS <= start_addr && end_addr < SCP_SCF_RAM_ADDRESS + SCP_EXP_TOP_SCF_RAM_SIZE) ||
        (SCP_TOP_SCP_DATA_RAM_ADDRESS <= start_addr && end_addr < SCP_TOP_SCP_DATA_RAM_ADDRESS + SCP_TOP_SCP_DATA_RAM_SIZE) ||
        (SCP_EXP_RAM0_ADDRESS <= start_addr && end_addr < SCP_EXP_RAM0_ADDRESS + SCP_EXP_TOP_RAM0_SIZE) ||
        (SCP_EXP_RAM1_ADDRESS <= start_addr && end_addr < SCP_EXP_RAM1_ADDRESS + SCP_EXP_TOP_RAM1_SIZE))
    {
        return true;
    }

    return false;
}
