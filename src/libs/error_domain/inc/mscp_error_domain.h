//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mscp_error_domain.h
 * Header file of error domain
 */
#pragma once

/*----------- Nested includes ------------*/
#include <atu_lib.h>
#include <cper.h>
#include <fpfw_icc_base.h>
#include <kng_atu_mappings.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
#ifdef SCP_RUNTIME_INIT
typedef enum
{
    SCP_S_ARSM_RAM = 0,
    SCP_NS_ARSM_RAM,
    SCP_RT_ARSM_RAM,
    SCP_RL_ARSM_RAM,
    SCP_ARSM_RAM_COUNT
} scp_arsm_ram_type_t;
#endif

/*-- Declarations (Statics and globals) --*/

#if defined (SCP_RUNTIME_INIT)
/*--------- Function Prototypes ----------*/
/**
 * @brief Register the SCP error domain.
 */
void register_scp_error_domain();

/**
 * @brief Get Shared ATU entry for Shared SRAM ECC Registers.
 */
void get_shared_sram_ecc_atu_entry(scp_arsm_ram_type_t type, atu_map_entry_t* atu_entry);

/**
 * @brief Get IRQ number for SCP ECC ISR based on the type of ARSM RAM.
 */
uint32_t get_irq_num_for_scp_ecc_isr(scp_arsm_ram_type_t type);

#elif defined (MCP_RUNTIME_INIT)
/**
 * @brief Register the MCP error domain.
 */
void register_mcp_error_domain(fpfw_icc_base_ctx_t* icc_ctx);

/**
 * @brief start ICC listener for MCP error injection request
 */
void start_mcp_error_injection_listener(fpfw_icc_base_ctx_t* icc_ctx);

/**
 * @brief Get MHU handle
 *  
 * @return
 *      pointer to handle.
 */
fpfw_icc_base_ctx_t* get_mhu_handle(void);
#endif