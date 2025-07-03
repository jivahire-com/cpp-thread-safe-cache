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
typedef enum
{
    MSCP_S_ARSM_RAM = 0,
    MSCP_NS_ARSM_RAM,
    MSCP_RT_ARSM_RAM,
    MSCP_RL_ARSM_RAM,
    MSCP_ARSM_RAM_COUNT
} mscp_arsm_ram_type_t;

typedef enum
{
    SCP_S_RSM_RAM = 0,
    SCP_NS_RSM_RAM,
    SCP_RSM_RAM_COUNT
} scp_rsm_ram_type_t;

/*--------- Function Prototypes ----------*/
/**
 * @brief Get Shared ATU entry for Shared SRAM ECC Registers.
 */
void get_shared_sram_ecc_atu_entry(mscp_arsm_ram_type_t type, atu_map_entry_t* atu_entry);

/**
 * @brief Get Shared ATU entry for Shared SRAM, RSM, ECC Registers.
 */
void get_rsm_ecc_atu_entry(scp_rsm_ram_type_t type, atu_map_entry_t* atu_entry);


#if defined (SCP_RUNTIME_INIT)
/**
 * @brief Register the SCP error domain.
 */
void register_scp_error_domain();
/**
 * @brief Register the SCP error domain.
 */
void register_pex_error_domain();

/**
 * @brief Get Shared ATU entry for Shared SRAM, ARSM, ECC Registers.
 */
void get_arsm_ecc_atu_entry(mscp_arsm_ram_type_t type, atu_map_entry_t* atu_entry);

/**
 * @brief Get IRQ number for SCP ECC ISR based on the type of ARSM RAM.
 */
uint32_t get_irq_num_for_scp_ecc_isr(mscp_arsm_ram_type_t type);

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