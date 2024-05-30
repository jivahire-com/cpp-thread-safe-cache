//
// Copyright (c) Microsoft Corporation.
//

/**
 * @file boot_status_codes.h
 * boot status code definitions for scp/mcp
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
//Post codes taken as it is from Pioneer

// SCP range is from 0x40 - 0x5F, and MCP is 0x60 - 0x7F
typedef enum _boot_status_code_t
{
    BOOT_STATUS_CODE_SCP_OK = 0x40,
    BOOT_STATUS_CODE_SCP_START,
    BOOT_STATUS_CODE_SCP_COLD_BOOT,
    BOOT_STATUS_CODE_SCP_WARM_BOOT,
    BOOT_STATUS_CODE_SCP_IRQ_DISABLED,
    BOOT_STATUS_CODE_SCP_E_SIZE,
    BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG,
    BOOT_STATUS_CODE_SCP_E_DATA_SRC_BASE,
    BOOT_STATUS_CODE_SCP_E_IMAGE_SIZE,
    BOOT_STATUS_CODE_SCP_E_ITCRAM_BASE,
    BOOT_STATUS_CODE_SCP_E_ITCRAM_SIZE,
    BOOT_STATUS_CODE_SCP_E_DTCRAM_BASE,
    BOOT_STATUS_CODE_SCP_E_DTCRAM_SIZE,
    BOOT_STATUS_CODE_SCP_E_SEND_HSP_FAILURE,
    BOOT_STATUS_CODE_SCP_E_BOOT_META,
    BOOT_STATUS_CODE_SCP_MAX = 0x5F,

    BOOT_STATUS_CODE_MCP_OK = 0x60,
    // Add as needed for MCP
    BOOT_STATUS_CODE_MCP_START,
    BOOT_STATUS_CODE_MCP_COLD_BOOT,
    BOOT_STATUS_CODE_MCP_WARM_BOOT,
    BOOT_STATUS_CODE_MCP_IRQ_DISABLED,
    BOOT_STATUS_CODE_MCP_E_SIZE,
    BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG,
    BOOT_STATUS_CODE_MCP_E_DATA_SRC_BASE,
    BOOT_STATUS_CODE_MCP_E_IMAGE_SIZE,
    BOOT_STATUS_CODE_MCP_E_ITCRAM_BASE,
    BOOT_STATUS_CODE_MCP_E_ITCRAM_SIZE,
    BOOT_STATUS_CODE_MCP_E_DTCRAM_BASE,
    BOOT_STATUS_CODE_MCP_E_DTCRAM_SIZE,
    BOOT_STATUS_CODE_MCP_E_SEND_HSP_FAILURE,
    BOOT_STATUS_CODE_MCP_E_BOOT_META,
    BOOT_STATUS_CODE_MCP_MAX = 0x7F

} boot_status_code_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
