//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_overrides_mcp.c
 * File containing all crash dump override functions for the framework
 */

/*------------- Includes -----------------*/
#include <stdint.h> // for uint32_t, uint64_t

/*-- Symbolic Constant Macros (defines) --*/
#define CD_DEFAULT_MEM_POOL_SIZE 128

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint8_t s_cd_bock_pool_memory[CD_DEFAULT_MEM_POOL_SIZE] = {};

/*------------- Functions ----------------*/

/*------- Memory Pool Overrides -------*/
/**
 * @brief Get the crash dump mem pool object
 *        ToDo: Will re-visit once MCP crash dump memory pool region is defined.
 *
 * @param mem_pool_addr
 * @param size
 */
void get_crash_dump_mem_pool(uint64_t* mem_pool_addr, uint32_t* size)
{
    *mem_pool_addr = (uint64_t)((uintptr_t)s_cd_bock_pool_memory);
    *size = CD_DEFAULT_MEM_POOL_SIZE;
}
