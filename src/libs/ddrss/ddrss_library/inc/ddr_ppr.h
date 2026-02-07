//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ddr_ppr.h
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
#include <ddrss_sdl.h>
#include <ddrss_runtime_api.h>
#include <idsw_kng.h>
#include <silibs_platform.h>
#include <fpfw_icc_base.h>        // for fpfw_icc_base_ctx_t
#include <hsp_firmware_headers.h> // for kng_hsp_mailbox_msg
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <variable_services.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PPR_RUN_CFG_VAR_NAME                    \
    {                                           \
        'P', 'P', 'R', '_', 'R', 'U', 'N', '\0' \
    }
#define PPR_RUN_CFG_VAR_GUID                               \
    {                                                      \
        0xa3fcb701, 0x4e5f, 0x44db,                        \
        {                                                  \
            0x93, 0x2d, 0x8b, 0x2f, 0xca, 0x10, 0xd9, 0x45 \
        }                                                  \
    }

#define DDR_PPR_VALID_DATA (0x9820U) // Address of Microsoft Hillsboro campus

// For logging SEL after PPR
#define SEL_RECORD_TYPE_DDR_PPR 0xCB
#define MPPR_COMPLETE_PASSED (0x10)

/*-------------- Typedefs ----------------*/

typedef struct ddr_ppr_sync_msg
{
    uint8_t ppr_type;      // PPR run type (DDRSS_PPR_TYPE)
    uint16_t valid;        // Valid flag true when D0 sets to DDR_PPR_VALID_DATA
    uint16_t ack;          // Ack flag set by D1 to acknowledge receipt
    uint8_t D1_ppr_complete; // D1 indicates when its PPR is complete
    uint8_t padding[10];   // Padding to reach minimum 16-byte message size
} ddr_ppr_sync_msg_t;

typedef enum
{
    E_PPR_STATUS_UPDATE_OK = 0,
    E_PPR_STATUS_UPDATE_INVALID_VENDOR_ID,
    E_PPR_STATUS_UPDATE_SPD_READ_FAIL,
    E_PPR_STATUS_UPDATE_SPD_WRITE_FAIL,
    E_PPR_STATUS_UPDATE_SEL_LOG_FAIL,
    E_PPR_STATUS_UPDATE_MAX = 0xFF,
} e_ppr_status_update_t;

typedef enum
{
    E_PPR_STATUS_NO_REPAIR = 0,
    E_PPR_STATUS_PASSED = 1,
    E_PPR_STATUS_FAILED = 2,
    E_PPR_STATUS_FAILED_OVERFLOW = 3,
    E_PPR_STATUS_FAILED_TEMPERATURE = 4,
    E_PPR_STATUS_MAX = 0xFF,
} e_ppr_spd_status_t;
typedef enum
{
    E_DTR_STATUS_NO_REPAIR = 0,
    // Todo - Talk to Adam & look at PPR_Azure spec for generic FAILURE
    E_DTR_STATUS_FAILURE_PPR_DONE_REPAIR_PASSED,
    E_DTR_STATUS_FAILURE_FAIL_OVERFLOW_REPAIR_FAILED_PPR,
    E_DTR_STATUS_FAILURE_TEMP_OVERFLOW,
    E_DTR_STATUS_FAILURE_PPR_FAIL_REPAIR_FAILED_HIGH_TEMPERATURE ,
    E_DTR_STATUS_FAILURE_SPPR_DONE,
    E_DTR_STATUS_PENDING_PPR,
    E_DTR_STATUS_INVALID = 0xFF,
} e_ppr_sel_status_t;

// Overall struct is 64-bits in size
typedef struct {
    uint64_t socket : 4;    //up to 16
    uint64_t ch:4;          //up to 16
    uint64_t dimm:1;        //primary/secondary 2dpc
    uint64_t rank:2;        //front/back
    uint64_t subrank:3;     //could be 8h 3ds
    uint64_t subCh:1;       //left/right
    uint64_t bankGroup:3;   //8 bank groups
    uint64_t bank:2;        //4 banks
    uint64_t row:17;        //0-16 (no 64Gb support)
    uint64_t DeviceMask:10; //Device Mask for 10 DRAM / subCh
    uint64_t temp:8;        //-40 to 100 
    uint64_t resrved:9;
} ddrss_spd_addr_info_t;

static_assert(sizeof(ddrss_spd_addr_info_t) == 8, "Size of ddrss_spd_addr_info_t must be 8 bytes");
static_assert(alignof(ddrss_spd_addr_info_t) == alignof(uint64_t), "Alignment of ddrss_spd_addr_info_t must match uint64_t");

typedef struct
{
    uint8_t spd_version;
    uint8_t ppr_count;
    uint8_t status;
    uint8_t ppr_fail_cnt;
    uint8_t ppr_exec_cnt;
} ddrss_spd_ppr_info_t;

typedef struct
{
    e_ppr_sel_status_t sel_ppr_status;
    e_ppr_spd_status_t spd_ppr_status;
    uint8_t num_repair_rows;
} ddrss_res_info_t;

/* Internal parameters calculated from configurations */

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
// PPR VAR in ddr_ppr.c
int32_t get_ppr_run_var_sync(void* ppr_type_req);
void ppr_reset_variable_cb(void* context, var_service_req_ctx_t* var_serv_ctx, uint8_t* data_start_ptr, size_t data_size);
void ppr_type_reset_variable(void);

void send_ppr_type_to_die1(DDRSS_PPR_TYPE ppr_type, volatile ddr_ppr_sync_msg_t* ppr_sync_msg);
void receive_ppr_type_from_die0(DDRSS_PPR_TYPE* ppr_type, volatile ddr_ppr_sync_msg_t* ppr_sync_msg);

void ppr_setup(ddrss_cfg_knobs_t* ddrss_cfgs);

// void ppr_update_sdl(ddrss_cfg_knobs_t* ddrss_cfgs);
void ppr_get_sync_msg_ptr(volatile ddr_ppr_sync_msg_t** ppr_sync_msg);
// void d1_send_sdl_update_complete(ddr_ppr_sync_msg_t* ppr_sync_msg);
// void d0_wait_for_d1_sdl_update_complete(ddr_ppr_sync_msg_t* ppr_sync_msg);

int32_t ppr_log_results_spd_sel(ddrss_spd_addr_info_t* addr_info, ddrss_res_info_t* res_info);
// int32_t ppr_log_results(ddrss_cfg_knobs_t* ddrss_cfgs);

void defect_to_addr_info(ddrss_addr_t* defect, uint32_t sdl_matching_idx, ddrss_spd_addr_info_t* ppr_spd_addr_info);
void defect_to_res_info(ddrss_addr_t* defect, ddrss_res_info_t* res_info);

ddrss_addr_t* ppr_get_defect_array_ptr(void);
bool is_defect_for_die(DIE_INSTANCE die_id, uint8_t MemoryControllerId);

int ddrss_convert_sdl_info_from_mc_to_dimm (MEMORY_DEFECT_V2* sdl_defect, ddrss_spd_addr_info_t *ppr_spd_addr_info);
int32_t ddrss_update_ppr_completion(ddrss_spd_addr_info_t *addr_info, ddrss_res_info_t *res_info);

volatile ddr_ppr_sync_msg_t* get_ppr_sync_msg_ptr(void);