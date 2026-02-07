//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ddrss_sdl.h
 */

#pragma once


/*----------- Nested includes ------------*/
#include <stdint.h>
#include <cper.h>
#include <ddr_erg0_regs.h>
#include <ddr_erg1_regs.h>
#include <ddr_manager.h>
#include <ddrmctop_regs.h>
#include <ddrss_knobs.h>
#include <ddrss_runtime_api.h>
#include <idsw_kng.h>
#include <silibs_platform.h>
#include <fpfw_icc_base.h>        // for fpfw_icc_base_ctx_t
#include <hsp_firmware_headers.h> // for kng_hsp_mailbox_msg
#include <stdbool.h>
#include <stdint.h>
#include <variable_services.h>

/*-- Symbolic Constant Macros (defines) --*/

// This variable is to communicate a pointer to the DDR location that SCP will use hand off the SDL to OS
#define SHARED_PAGE_BASE_NAME \
    {                \
        'S', 'h', 'a', 'r', 'e', 'd', 'P', 'a', 'g', 'e', 'B', 'a', 's', 'e', '\0' \
    }

#define SHARED_PAGE_BASE_GUID \
    {                                                      \
        0xFF6A5919, 0x380C, 0x4A75,                        \
        {                                                  \
            0xB3, 0xCE, 0xE2, 0x8D, 0xB2, 0x24, 0x6A, 0xF0 \
        }                                                  \
    }

// This variable represents the non-volatile storage of SDL across reboots
#define SDL_VAR_NAME  {'S', 'D', 'L', '\0' }
#define SDL_VAR_GUID  {0xD369C0CF, 0x8935, 0x44A3, { 0x81, 0xF7, 0xAE, 0x9A, 0x4A, 0xDE, 0x9F, 0x8F }}
#define SDL_MAX_SIZE (0x20000) // 128KB

#define MEMORY_DEFECT_LIST_VERION_10 0x00000001
#define MEMORY_DEFECT_VERSION_10 0x0001
#define MEMORY_DEFECT_VERSION_20 0x0002

#define PSHED_PI_DEFECT_LIST_SIGNATURE 0x4C444D53U // 'SMDL' in ASCII (little-endian)

#define PPR_COMPLETION_STATUS_DETAIL_OVERFLOW 0x00000001
#define PPR_COMPLETION_STATUS_DETAIL_TEMPERATION 0x00000002

#define SDL_VAR_SIZE_BYTES 0x20000 // 128KB

// SDL Schema related
#define KNG_NUM_SOCKETS          (1)
#define KNG_MAX_CHANNELS         (12)
#define KNG_MAX_SUBCHANNELS_PER_CHANNEL  (2)
#define KNG_MAX_RANK (2)
#define KNG_MAX_BG (7)
#define KNG_MAX_BANK (2)
#define KNG_MAX_ROW (0x3FFFF)
#define KNG_MAX_COL (0x7FF)
#define KNG_MAX_DEVICES (64) //? 64 Devices/Electrical rank???

#define KNG_MAX_SUBCHANNELS      (2)


/*-------------- Typedefs ----------------*/
typedef struct _MEMORY_DEFECT_LIST_HEADER
{
    uint32_t Signature;
    uint32_t Version;
    uint32_t Length;
    uint32_t DefectCount;
    uint32_t Checksum;
    uint32_t Changed;
    uint32_t Reserved1;
    uint32_t Reserved2;
} MEMORY_DEFECT_LIST_HEADER, *PMEMORY_DEFECT_LIST_HEADER;

typedef struct _DIMM_ADDR_VALID_BITS_DDR4
{
    uint32_t SocketId: 1;
    uint32_t MemoryControllerId: 1;
    uint32_t ChannelId: 1;
    uint32_t DimmSlot: 1;
    uint32_t DimmRank: 1;
    uint32_t Device: 1;
    uint32_t ChipSelect: 1;
    uint32_t Bank: 1;
    uint32_t Dq: 1;
    uint32_t Row: 1;
    uint32_t Column: 1;
    uint32_t Info: 1;
    uint32_t Reserved: 20;
} DIMM_ADDR_VALID_BITS_DDR4, *PDIMM_ADDR_VALID_BITS_DDR4;

typedef struct _DIMM_ADDR_VALID_BITS_DDR5
{
    uint32_t SocketId: 1;
    uint32_t MemoryControllerId: 1;
    uint32_t ChannelId: 1;
    uint32_t SubChannelId: 1;
    uint32_t DimmSlot: 1;
    uint32_t DimmRank: 1;
    uint32_t Device: 1;
    uint32_t ChipId: 1;
    uint32_t Bank: 1;
    uint32_t Dq: 1;
    uint32_t Row: 1;
    uint32_t Column: 1;
    uint32_t Info: 1;
    uint32_t Reserved: 19;
} DIMM_ADDR_VALID_BITS_DDR5, *PDIMM_ADDR_VALID_BITS_DDR5;

typedef union _DIMM_ADDRESS
{
    //
    // DDR4 Address
    //
    struct
    {
        uint64_t SocketId: 4;            // 16 Sockets
        uint64_t MemoryControllerId: 2;  // 4 Memory Controllers
        uint64_t ChannelId: 2;           // 4 Channels
        uint64_t DimmSlot: 2;            // 3 DIMMs
        uint64_t DimmRank: 2;            // 4 Ranks
        uint64_t Device: 5;              // 18 Devices
        uint64_t ChipSelect: 3;          // 8 Chip IDs
        uint64_t Bank: 8;                // 16 Banks-includes BankGroup and Bank
        uint64_t Dq: 4;                  // 16 DQs
        uint64_t Reserved: 32;
        uint32_t Row;
        uint32_t Column;
        uint64_t Info;
    } Ddr4;

    //
    // DDR5 Address
    //
    struct
    {
        uint64_t SocketId: 5;            // Up to 32 Sockets   Prod boares some have 2 sockets - may able to read from fuse bits?  can keep 0
        uint64_t MemoryControllerId: 5;  // Up to 32 Memory Controllers/Socket: 0-23:  ddrss_index = This ID/2
        uint64_t ChannelId: 3;           // Up to 8 Channels/Memory Controller.  Redundant
        uint64_t SubChannelId: 2;        // 4 Subchannels/Channel.  Redundant
        uint64_t DimmSlot: 2;            // Up to 4 DIMMs/(Subchannel/Channel)  Always 0.  Not used for ddrss
        uint64_t DimmRank: 4;            // Up to 16 Electrical ranks/DIMM:  equivalent to 49 (.rank)
        uint64_t Device: 6;              // Up to 64 Devices/Electrical rank: 
        uint64_t ChipId: 4;              // Up to 16 Chip IDs/DRAM Device
        uint64_t Bank: 16;               // 256 Banks-includes BankGroup and Bank:  .bg = (Bank >> 8) & 7  .bank = (Bank & 0xFF) & 3
        uint64_t Dq: 10;                 // DQ = Lane:  This is 53 (ppr_device_mask)
        uint64_t Reserved: 7;
        uint32_t Row;                     // Up to 18 Row Bits:  This is 45 - .row
        uint32_t Column;                  // Up to 11 Column Bits: 46
        uint64_t Info;
    } Ddr5;
} DIMM_ADDRESS, *PDIMM_ADDRESS;

typedef union _DIMM_ADDR_VALID_BITS
{
    DIMM_ADDR_VALID_BITS_DDR4 VB_DDR4;
    DIMM_ADDR_VALID_BITS_DDR5 VB_DDR5;
    uint32_t AsUINT32;
} DIMM_ADDR_VALID_BITS, *PDIMM_ADDR_VALID_BITS;

typedef struct _DIMM_INFO
{
    DIMM_ADDRESS DimmAddress;
    DIMM_ADDR_VALID_BITS ValidBits;
} DIMM_INFO, *PDIMM_INFO;

// Each defect is defined as follows: 
typedef enum _PAGE_OFFLINE_ERROR_TYPES
{
    BitErrorDdr4,
    RowErrorDdr4,
    BitErrorDdr5,
    RowErrorDdr5
} PAGE_OFFLINE_ERROR_TYPES, *PPAGE_OFFLINE_ERROR_TYPES;

typedef struct _MEMORY_DEFECT_FLAGS
{
    uint16_t RepairedDefectEntry: 1;
    uint16_t Reserved: 15;
} MEMORY_DEFECT_FLAGS, *PMEMORY_DEFECT_FLAGS;

typedef struct _MEMORY_DEFECT_FLAGS_V2
{
    uint16_t DefectRepairRequested: 1;
    uint16_t DefectRepairAttempted: 1;
    uint16_t Reserved: 14;
} MEMORY_DEFECT_FLAGS_V2, *PMEMORY_DEFECT_FLAGS_V2;

typedef struct _MEMORY_DEFECT
{
    uint16_t Version;
    MEMORY_DEFECT_FLAGS Flags;
    DIMM_INFO DimmInfo;
    PAGE_OFFLINE_ERROR_TYPES ErrType;
} MEMORY_DEFECT, *PMEMORY_DEFECT;

typedef enum PPR_COMPLETION_STATUS
{
    NoRepair = 0,
    Passed,
    Failed
} PPR_COMPLETION_STATUS;

typedef struct _PPR_COMPLETION_INFO
{
    uint32_t Status;
    uint32_t Details;
} PPR_COMPLETION_INFO, *PPPR_COMPLETION_INFO;

typedef struct _MEMORY_DEFECT_V2
{
    uint16_t Version;
    MEMORY_DEFECT_FLAGS_V2 Flags;
    DIMM_INFO DimmInfo;
    PAGE_OFFLINE_ERROR_TYPES ErrType;
    PPR_COMPLETION_INFO PprCompletionInfo;
} MEMORY_DEFECT_V2, *PMEMORY_DEFECT_V2;

typedef enum _SDL_ADDRESS_PARAM
{
    Param_SocketId = 1,
    Param_MemoryControllerId,
    Param_ChannelId,
    Param_SubChannelId,
    Param_DimmSlot,
    Param_DimmRank,
    Param_Device,
    Param_ChipId,
    Param_Bank,
    Param_Dq,
    Param_Row,
    Param_Column,
    Param_Info,
    Param_Max
} SDL_ADDRESS_PARAM;

/* Internal parameters calculated from configurations */

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/* SDL Flash/Variable Storage */
int32_t load_sdl_from_flash(uintptr_t load_addr);
void sdl_get_mem_ctx(var_service_shared_mem_t* mem_ctx);
void store_sdl_var_async(void *ssi_request);

/* SDL Initialization and Setup */
void copy_empty_sdl_header_to_reserved_ddr(uintptr_t dest_addr_ddr);
void sdl_map_atu(uint64_t base_addr);
void sdl_unmap_atu(void);
uintptr_t sdl_get_atu_start_addr(void); // Change
uintptr_t get_sdl_arsm0_addr(void);
void load_shared_defect_list_to_DDR(void);
void ddr_publish_sdl_addr(void);
void cleanup_after_sdl_load(void);

/* SDL Parsing and Validation */
void parse_sdl_var(uintptr_t sdl_base, uintptr_t defect_list_addr);
int32_t sdl_has_valid_entries(uintptr_t sdl_base);
size_t sdl_calculate_size_by_defect_count(uintptr_t sdl_base);
size_t sdl_get_size_from_header(uintptr_t sdl_base);
bool sdl_signature_is_valid(uintptr_t sdl_base);

/* SDL Update and Maintenance */
void sdl_update_checksum(uintptr_t sdl_base);
void store_sdl_var_sync_from_arsm0(void);

/* PPR Defect List Operations */
int build_ppr_defect_list(ddrss_cfg_knobs_t* ddrss_cfgs, uintptr_t sdl_base);

/* SDL Address Translation */
void sdl_map_atu(uint64_t base_addr);
void sdl_unmap_atu(void);
