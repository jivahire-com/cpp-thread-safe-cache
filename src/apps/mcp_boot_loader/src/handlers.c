//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file handlers.c
 * This file contains the reset handler for the MCP boot loader.
 */

/*------------- Includes -----------------*/
#include <kingsgate_boot.h> // for kingsgate_boot_metadata_t, BITMASK_WARM_...
#ifdef UNIT_TEST
    #include <mock_mcp_mmap.h>
#else
    #include <mcp_mmap.h> // for MCP_DTCM_RAM_SIZE, MCP_MSCP_EXP_SRAM0_ADDR, MCP_TOP_MCP_DATA_RAM_ADDRESS, MCP_TOP_MC...
#endif
#include <stdbool.h> // for bool
#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint32_t
#ifdef UNIT_TEST
    #define NORETURN
#else
    #include <stdnoreturn.h> // for noreturn
    #define NORETURN noreturn
#endif
/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------- Function Prototypes -----------*/

extern NORETURN void _start(void);

/*------------- Functions ----------------*/

/**
 *  @brief This function is used to clear the ITCM and DTCM before invoking _start
 *
 *  @note This function is invoked from the reset handler and is the entry point of the boot loader.
 *
 *  @param[in] None
 *  @param[out] None
 *
 *  @return
 *      This is a no return function .
 */
NORETURN void arch_exception_reset(void)
{
    kingsgate_boot_metadata_t* boot_metadata = (kingsgate_boot_metadata_t*)(MCP_MSCP_EXP_SRAM0_ADDR);

    // TODO: Need to profile ITCM/DTCM clear and see if its possible to optimize using memset/DMA
    // ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1777866/?triage=true
    /*
     * NOLINT is set to skip LINT checks for the ITCM clear, this for two reasons
     *   1. The ITCM starts from address 0x0, linter will raise a null dereference error even though this is legal
     *   2. Getting -Wint-to-pointer-cast error when trying to write 0 to the ITCM address, this is also being suppressed
     */
    for (uint32_t mem_offset = 0; mem_offset < MCP_TOP_MCP_INST_RAM_SIZE; mem_offset += sizeof(uint32_t))
    {
        *(volatile uint32_t*)((size_t)MCP_TOP_MCP_INST_RAM_ADDRESS + mem_offset) = (uint32_t)0x0; // NOLINT
    }

    // NOTE: Currently HSP doesn't download metadata and reset reason will always be cold boot
    bool is_warm_boot_detected = boot_metadata->reset_reason & BITMASK_WARM_BOOT;

    if (!is_warm_boot_detected)
    {
        for (uint32_t mem_offset = 0; mem_offset < MCP_TOP_MCP_DATA_RAM_SIZE; mem_offset += sizeof(uint32_t))
        {
            // NOLINT added as a precaution against -Wint-to-pointer-cast error
            *(volatile uint32_t*)((size_t)MCP_TOP_MCP_DATA_RAM_ADDRESS + mem_offset) = 0x0;
        }
    }

    // Invoke _start from crt0.S which will finally invoke main
    _start();
}