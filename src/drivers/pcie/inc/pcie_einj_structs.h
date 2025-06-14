//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_einj_structs.h
 * Macros and definitions for common PCIe error injection structures.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <assert.h>
#include <pcie_rp_rasdes.h>
#include <pcie_ss.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
typedef enum
{
    PCIE_ERROR_TYPE_AER = 0,
    PCIE_ERROR_TYPE_RP_INTERNAL_CRC = 1,
    PCIE_ERROR_TYPE_RP_INTERNAL_SEQNUM = 2,
    PCIE_ERROR_TYPE_RP_INTERNAL_DLLP = 3,
    PCIE_ERROR_TYPE_RP_INTERNAL_SYMBOL = 4,
    PCIE_ERROR_TYPE_RP_INTERNAL_FC = 5,
    PCIE_ERROR_TYPE_RP_INTERNAL_RETRY_TLP = 6,
    PCIE_ERROR_TYPE_MAX
} PCIE_ERROR_TYPE;

typedef union
{
    PCIE_SS_RP_APP_ERROR aer;
    union
    {
        PCIE_RASDES_INJ_CRC_TYPE crc;
        PCIE_RASDES_INJ_SEQNUM_TYPE seqnum;
        PCIE_RASDES_INJ_DLLP_TYPE dllp;
        PCIE_RASDES_INJ_SYMBOL_TYPE symbol;
        PCIE_RASDES_INJ_FC_TYPE fc;
        PCIE_RASDES_INJ_RETRY_TLP_TYPE retry_tlp;
    } internal;
} PCIE_ERROR_DATA;

typedef struct
{
    uint32_t reserved8 : 8;
    uint32_t function : 3;
    uint32_t device : 5;
    uint32_t bus : 8;
    uint32_t segment : 8;
} pcie_sbdf_t;

typedef struct _pcie_einj_params
{
    PCIE_ERROR_TYPE error_type;
    PCIE_ERROR_DATA error_data;
    uint8_t error_count;
    pcie_sbdf_t sbdf;
} pcie_einj_params_t;

static_assert(sizeof(pcie_einj_params_t) <= (sizeof(uint64_t) * 2), "PCIe params overflows 16 bytes (max allowed)!");
