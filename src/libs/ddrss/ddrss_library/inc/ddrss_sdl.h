//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ddrss.h
 */

#pragma once


/*    Outline    Will delete once full feature is implemented

Normal boot:
1. SCP firmware will read "RUN_PPR" variable and see it is set to NONE
(done)2. SCP firmware will read SDL variable from HSP/flash and load to DDR at the reserved address in ddrss_reserved_regions.h
(done)2a.  If SDL not found, SCP will create an empty SDL "Header" and copy that to DDR
(done)3. SCP firmware will create a UEFI variable with the location of the reserved region that holds the SDL as a hand-off to OS
4. At shutdown/reboot, SCP will read the latest SDL from DDR and write back to flash via HSP.
- Register for a reboot/shutdown callback to do this.

HPPR boot:
1. SCP firmware will read "RUN_PPR" variable and see it is set to HPPR
 a. Clear the RUN_PPR variable to NONE after reading it to avoid repeated HPPR on next boot
2. SCP firmware will read SDL variable from HSP/flash and load to DDR *** to ARSM0 *** (it is accessible by both dies)
3. Both dies will parse the SDL in ARSM0 and build defect lists to give to their local SiLibs 
4. Both dies will call ddr_init as usual, but with the PPR flag set in ddrss_cfgs.ext_knobs.ppr_type and the defect lists populated
5. After DDR init (and PPR is completed), SCP firmware needs to:
    a. Update the SDL with the status of the PPR (which defects were applied, which were not, etc)
    b. Write the updated SDL back to flash via HSP (and copy the updated SDL to DDR for OS hand-off)
    c. Write the PPR addresses to each DIMM's SPD region over I3C
    d. Write SEL for each PPR event that goes to BMC
** If we continue to boot **
6. Copy latest SDL after PPR updates to DDR (same as normal boot)
??? Can both SCP dies update SDL in ARSM0 simultaneously since they are updating their own sections of it?  
? And/Or do we need a sync point?  (Yes) to know when Die 1 is finished so we can write updated SDL back to flash (and to DDR)

MPPR boot:
1. This is much easier.  We see that the "RUN_PPR" variable is set to MPPR
2. SCP firmware does not use SDL variable at all
3. SCP firmware sets the ddrss_cfgs.ext_knobs.ppr_type to MPPR
4. Calls ddr_init as usual, SiLibs will do MPPR based on its internal logic
5. After DDR init, SCP firmware does not need to update SDL since not used
6. Write SEL for each MPPR event that goes to BMC
7. If available, update each DIMM's SPD region over I3C with repaired addresses

ToDo:
- Verify that die to die ICC messages are working for PPR type communication
(done)- Verify that CLI commands to write "RUN_PPR" variable are working
(done)- Veriif that CLI commands to read "RUN_PPR" variable are working
- Test SPD writes over I3C updates
- Test SEL entries for PPR/MPPR events - Make sure that BMC is expecting them and doesn't drop them.
(done)- Load SDL variable from flash via HSP and copy to DDR for normal boot
- Implement shutdown/reset hook/callback to copy SDL from DDR back to flash.
*/








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

#define MEMORY_DEFECT_LIST_VERION_10 0x00000001
#define MEMORY_DEFECT_VERSION_10 0x0001
#define MEMORY_DEFECT_VERSION_20 0x0002

#define PSHED_PI_DEFECT_LIST_SIGNATURE "SMDL" // 'SMDL' in ASCII

#define PPR_COMPLETION_STATUS_DETAIL_OVERFLOW 0x00000001
#define PPR_COMPLETION_STATUS_DETAIL_TEMPERATION 0x00000002

// Todo -- Is this a max entry count?  Or is it 128KB total?
#define SHARED_DEFECT_LIST_MAX_ENTRIES 495  // this is not right for 128KB size  TODO...
#define SDL_VAR_SIZE_BYTES 0x20000 // 128KB


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
        uint64_t SocketId: 5;            // Up to 32 Sockets
        uint64_t MemoryControllerId: 5;  // Up to 32 Memory Controllers/Socket
        uint64_t ChannelId: 3;           // Up to 8 Channels/Memory Controller
        uint64_t SubChannelId: 2;        // 4 Subchannels/Channel
        uint64_t DimmSlot: 2;            // Up to 4 DIMMs/(Subchannel/Channel)
        uint64_t DimmRank: 4;            // Up to 16 Electrical ranks/DIMM
        uint64_t Device: 6;              // Up to 64 Devices/Electrical rank
        uint64_t ChipId: 4;              // Up to 16 Chip IDs/DRAM Device
        uint64_t Bank: 16;               // 256 Banks-includes BankGroup and Bank
        uint64_t Dq: 10;                 // DQ = Lane
        uint64_t Reserved: 7;
        uint32_t Row;                     // Up to 18 Row Bits
        uint32_t Column;                  // Up to 11 Column Bits
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
int32_t get_sdl_var(uintptr_t load_addr);
void sdl_get_mem_ctx(var_service_shared_mem_t* mem_ctx);
void copy_empty_sdl_header_to_reserved_ddr(uintptr_t dest_addr_ddr);

int32_t get_num_valid_sdl_entries(uintptr_t sdl_base);
int32_t validate_sdl_addr_param(uint32_t param_value, SDL_ADDRESS_PARAM param_type);
void parse_sdl_var(uintptr_t sdl_base, uintptr_t defect_list_addr);
