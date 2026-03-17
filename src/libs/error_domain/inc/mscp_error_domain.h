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
#include <icc_mhu.h> // for icc_mhu_packet_t
#include <icc_platform_defines.h>

/*-- Symbolic Constant Macros (defines) --*/

// TO DO : remove below once the silib is updated
// https://dev.azure.com/ms-tsd/Kingsgate/_git/silibs/pullrequest/302473
#ifndef ICC_HM_ERROR_INJECTION_SETUP_REQ
#define ICC_HM_ERROR_INJECTION_SETUP_REQ 0x00070007
#endif

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
    MSCP_S_RSM_RAM = 0,
    MSCP_NS_RSM_RAM,
    MSCP_RSM_RAM_COUNT
} mscp_rsm_ram_type_t;

typedef struct
{
    icc_mhu_header_t header;
    uint32_t mcp_error_type;
} icc_mhu_mscp_err_injection_setup_packet_t;

/*--------- Function Prototypes ----------*/
/**
 * @brief Get Shared ATU entry for Shared SRAM, ARSM, ECC Registers.
 */
void get_arsm_ecc_atu_entry_die(mscp_arsm_ram_type_t type, uint8_t die_id, atu_map_entry_t* atu_entry);

/**
 * @brief Get Shared ATU entry for Shared SRAM, ARSM, ECC Registers in this die.
 */
void get_arsm_ecc_atu_entry(mscp_arsm_ram_type_t type, atu_map_entry_t* atu_entry);

/**
 * @brief Get Shared ATU entry for Shared SRAM, RSM, ECC Registers.
 */
void get_rsm_ecc_atu_entry_die(mscp_rsm_ram_type_t type, uint8_t die_id, atu_map_entry_t* atu_entry);

/**
 * @brief Get Shared ATU entry for Shared SRAM, RSM, ECC Registers in this die.
 */
void get_rsm_ecc_atu_entry(mscp_rsm_ram_type_t type, atu_map_entry_t* atu_entry);

/**
 * @brief Checks if the address is in cached space.
 * @param address
 * @return 
 *      true if address is in cached space, false otherwise.
 */
bool is_cached_space(uint32_t addr);

/**
 * @brief Get MHU handle
 *  
 * @return
 *      pointer to handle.
 */
fpfw_icc_base_ctx_t* get_mhu_handle(void);

/**
 * @brief Set MHU handle
 *  
 */
void set_mhu_handle(fpfw_icc_base_ctx_t* icc_ctx);

/**
 * @brief Clear poison memory, 8b clear is performed for the given address range.
 *  
 */
int clear_poison_memory(uintptr_t poison_addr);

#if defined (SCP_RUNTIME_INIT)
/**
 * @brief Register the SCP error domain.
 */
void register_scp_error_domain(fpfw_icc_base_ctx_t* icc_ctx);

/**
 * @brief Get IRQ number for SCP ECC ISR based on the type of ARSM RAM.
 */
uint32_t get_irq_num_for_scp_ecc_isr(mscp_arsm_ram_type_t type);

/**
 * @brief start ICC listener for MCP error injection setup request
 */
void mcp_error_injection_setup_listener(fpfw_icc_base_ctx_t* icc_ctx);

#elif defined (MCP_RUNTIME_INIT)
/**
 * @brief Register the MCP error domain.
 */
void register_mcp_error_domain(fpfw_icc_base_ctx_t* icc_ctx);

/**
 * @brief start ICC listener for MCP error injection request
 */
void start_mcp_error_injection_listener(fpfw_icc_base_ctx_t* icc_ctx);

void request_mcp_error_injection_setup(mcp_proc_err_type_t mcp_error_type, uint32_t trigger_addr);

#endif