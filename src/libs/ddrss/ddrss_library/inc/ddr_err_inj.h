//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_err_inj.h
 * Header file to support implementations of ddr CLI commands.
 */


/*------------- Includes -----------------*/
#include <ddrss_runtime_api.h>
#include <health_monitor.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


/*-- Symbolic Constant Macros (defines) --*/



#define ERG0 0
#define ERG1 1
#define DONTCARE 0

// Macro list for all error injection syndromes
#define DDR_ERR_INJ_SYNDROME_LIST(MACRO)                                                                     \
    MACRO(DDR_ERR_INJ_MREB_MAINLINE_TRAFFIC_CE,         ERG0, RAS_ARM_INJ_CE_PERSISTENT | RAS_ARM_INJ_AV, 0x0000000000000001, 1, 0x02, 0x0C, 0, 0, 0, 0, 1, 1, ddr_err_inj_mainline_traffic_ce) \
    MACRO(DDR_ERR_INJ_MREB_MAINLINE_TRAFFIC_UE,         ERG0, RAS_ARM_INJ_DE | RAS_ARM_INJ_AV, 0x0000000000000020, 1, 0x03, 0x0C, 0, 0, 0, 0, 1, 1, ddr_err_inj_mainline_traffic_ue) \
    MACRO(DDR_ERR_INJ_MREB_PATROL_SCRUB_CE,             ERG0, RAS_ARM_INJ_CE_PERSISTENT | RAS_ARM_INJ_AV, 0x0000000000000300, 1, 0x04, 0x0C, 0, 0, 0, 0, 1, 1, ddr_err_inj_media_patrol_scrub_ce) \
    MACRO(DDR_ERR_INJ_MREB_PATROL_SCRUB_UE,             ERG0, RAS_ARM_INJ_DE | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x05, 0x0C, 0, 0, 0, 0, 1, 1, ddr_err_inj_media_patrol_scrub_ue) \
    MACRO(DDR_ERR_INJ_FECQ_FEDB_DATA_ARRAY_UE,          ERG0, RAS_ARM_INJ_DE | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x21, 0x02, 0, 0, 0, 0, 1, 1, ddr_err_inj_fecq_fedb_data_array_ue) \
    MACRO(DDR_ERR_INJ_FECQ_FEDB_MERGE_DATA_UE,          ERG0, RAS_ARM_INJ_DE | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x21, 0x10, 0, 0, 0, 0, 1, 1, ddr_err_inj_fedb_merge_data_ue) \
    MACRO(DDR_ERR_INJ_FECQ_FEDB_MERGE_DATA_CE,          ERG0, RAS_ARM_INJ_CE_PERSISTENT | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x22, 0x10, 0, 0, 0, 0, 1, 1, ddr_err_inj_fedb_merge_data_ce) \
    MACRO(DDR_ERR_INJ_FECQ_FEDB_MERGE_DATA_PARITY_UE,   ERG0, RAS_ARM_INJ_DE | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x31, 0x11, 0, 0, 0, 0, 1, 1, ddr_err_inj_fedb_merge_data_parity_ue) \
    MACRO(DDR_ERR_INJ_FECQ_FEDB_MERGE_STROBE_UE,        ERG0, RAS_ARM_INJ_DE | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x32, 0x11, 0, 0, 0, 0, 1, 1, ddr_err_inj_fedb_merge_strobe_ue) \
    MACRO(DDR_ERR_INJ_FECQ_FEDB_MERGE_STROBE_PARITY_UE, ERG0, RAS_ARM_INJ_DE | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x31, 0x11, 0, 0, 0, 0, 1, 1, ddr_err_inj_fedb_merge_strobe_parity_ue) \
    MACRO(DDR_ERR_INJ_FECQ_FEDB_STROBE_ARRAY_UE,        ERG0, RAS_ARM_INJ_DE | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x31, 0x02, 0, 0, 0, 0, 1, 1, ddr_err_inj_fedb_strobe_array_ue) \
    MACRO(DDR_ERR_INJ_HKE_PERSISTENT_CA_PARITY_UE,      ERG0, RAS_ARM_INJ_UEU | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x03, 0x0B, 0, 0, 0, 0, 1, 1, ddr_err_inj_ca_parity_persistent) \
    MACRO(DDR_ERR_INJ_HKE_TRANSIENT_CA_PARITY_UE,       ERG0, RAS_ARM_INJ_CE_TRANSIENT | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x02, 0x0B, 0, 0, 0, 0, 1, 1, ddr_err_inj_ca_parity_transient) \
    MACRO(DDR_ERR_INJ_RH_COUNTERS_SRAM_PARITY,          ERG0, RAS_ARM_INJ_CE_PERSISTENT | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x41, 0x02, 0, 0, 0, 0, 1, 1, ddr_err_rh_counters_sram_parity) \
    MACRO(DDR_ERR_INJ_RH_DRFM_SRAM_PARITY,              ERG0, RAS_ARM_INJ_CE_PERSISTENT | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x42, 0x02, 0, 0, 0, 0, 1, 1, ddr_err_rh_drfm_sram_parity) \
    MACRO(DDR_ERR_INJ_XTS_AES_KEYSTORE_CE,              ERG0, RAS_ARM_INJ_CE_PERSISTENT | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x08, 0x11, 0, 0, 0, 0, 1, 1, ddr_err_xts_aes_keystore_ce) \
    MACRO(DDR_ERR_INJ_XTS_AES_KEYSTORE_UE,              ERG0, RAS_ARM_INJ_DE | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x09, 0x11, 0, 0, 0, 0, 1, 1, ddr_err_xts_aes_keystore_ue) \
    MACRO(DDR_ERR_INJ_BCP_READ_ADDR_NOT_IN_DDR,         ERG0, RAS_ARM_INJ_UER | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x01, 0x0D, 0, 0, 0, 0, 1, 1, ddr_err_bcp_read_addr_not_in_ddr) \
    MACRO(DDR_ERR_INJ_BCP_READ_BLOCKED_BY_PAS,          ERG0, RAS_ARM_INJ_UER | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x01, 0x0E, 0, 0, 0, 0, 1, 1, ddr_err_bcp_read_blocked_by_pas) \
    MACRO(DDR_ERR_INJ_BCP_WRITE_ADDR_NOT_IN_DDR,        ERG0, RAS_ARM_INJ_UEO | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x02, 0x0D, 0, 0, 0, 0, 1, 1, ddr_err_bcp_write_addr_not_in_ddr) \
    MACRO(DDR_ERR_INJ_BCP_WRITE_BLOCKED_BY_PAS,         ERG0, RAS_ARM_INJ_UEO | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x02, 0x0E, 0, 0, 0, 0, 1, 1, ddr_err_bcp_write_blocked_by_pas) \
    MACRO(DDR_ERR_INJ_BCP_CHI_UNSUPPORTED_OPCODE,       ERG1, RAS_ARM_INJ_UC | RAS_ARM_INJ_CI | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x01, 0x04, 0, 0, 0, 0, 1, 1, ddr_err_bcp_chi_unsupported_opcode) \
    MACRO(DDR_ERR_INJ_MRDP_PARITY_ERROR,                ERG1, RAS_ARM_INJ_UC | RAS_ARM_INJ_CI | RAS_ARM_INJ_AV, 0x0000000000000000, 1, 0x50, 0x11, 0, 0, 0, 0, 1, 1, ddr_err_inj_mrdp_parity_ue) \
    MACRO(DDR_ERR_INJ_ECC_CE,                           DONTCARE, DONTCARE | DONTCARE | DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, ddr_err_inj_ecc_ce) \
    MACRO(DDR_ERR_INJ_ECC_UE,                           DONTCARE, DONTCARE | DONTCARE | DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, DONTCARE, ddr_err_inj_ecc_ue) 

#define NAMED_SYNDROME_STRUCT_INIT(token, erg, ras_arm_err_bitmask, addr, inj_counter, ierr, serr, ns, nse, si, ai, va, valid, inj_func) \
    {#token, erg, ras_arm_err_bitmask, {addr, inj_counter, ierr, serr, ns, nse, si, ai, va, valid}, inj_func},

// Macro to generate enum values from syndrome tokens
#define SYNDROME_ENUM(token, ...) token,

/**
 * @brief Enum for DDR error injection syndrome indices
 * This enum provides indexed access to syndromes in the DDR_ERR_INJ_SYNDROME_LIST
 * Each token corresponds to its position in the list (0-based indexing)
 */
typedef enum {
    DDR_ERR_INJ_SYNDROME_LIST(SYNDROME_ENUM)
    DDR_ERR_INJ_SYNDROME_COUNT // Total count of syndromes
} ddr_err_inj_syndrome_index_t;

/*-------------- Typedefs ----------------*/

/**
 * @brief Function pointer type for error injection functions
 * @param mc Memory controller index
 */
typedef void (*ddr_err_inj_func_t)(uint32_t mc);

/**
 * @brief Enum for RAS injection syndrome types
 * This enum provides indexed access to the ras_syndrome array
 * The order matches the array definition in ddr_err_inj.c
 */
typedef struct _ddr_err_inj_syndrome
{
    const char* name; // Name of the syndrome
    uint8_t erg; // 0 or 1
    uint32_t ras_arm_err_bitmask; // RAS_ARM_ERROR_TYPE(s)
    ras_inj_syndrome_t syndrome; // Syndrome value
    ddr_err_inj_func_t inj_func; // Function pointer to error injection function
} ddr_err_inj_syndrome_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

void ddr_err_inj_ecc_ce(uint32_t mc);
void ddr_err_inj_ecc_ue(uint32_t mc);
void ddr_ecc_error_injection(int32_t die_num, uint32_t mc, uint64_t p_addr, uint16_t Bit);

void ddrss_err_inj_atu_map(uint32_t die_num);
void ddrss_err_inj_atu_unmap();
int ddr_ca_parity_error_injection(uint32_t mc, DDRSS_MEDIA_CA_INJ_CMD cmd);
bool ddr_ecc_err_inj_validation(uint32_t mc, uint16_t BIT);

void ddr_err_inj_media_patrol_scrub_ce(uint32_t mc);
void ddr_err_inj_media_patrol_scrub_ue(uint32_t mc);

void ddr_err_inj_fedb_merge_data_ce(uint32_t mc);
void ddr_err_inj_fedb_merge_data_ue(uint32_t mc);
void ddr_err_inj_fedb_merge_data_parity_ue(uint32_t mc);
void ddr_err_inj_fedb_merge_strobe_ue(uint32_t mc);
void ddr_err_inj_fedb_merge_strobe_parity_ue(uint32_t mc);
void ddr_err_inj_fedb_strobe_array_ue(uint32_t mc);
void ddr_err_inj_fecq_fedb_data_array_ue(uint32_t mc);
void ddr_err_inj_mainline_traffic_ce(uint32_t mc);
void ddr_err_inj_mainline_traffic_ue(uint32_t mc);

void ddr_err_inj_ca_parity_persistent(uint32_t mc);
void ddr_err_inj_ca_parity_transient(uint32_t mc);

void ddr_err_rh_counters_sram_parity(uint32_t mc);
void ddr_err_rh_drfm_sram_parity(uint32_t mc);

void ddr_err_xts_aes_keystore_ce(uint32_t mc);
void ddr_err_xts_aes_keystore_ue(uint32_t mc);

void ddr_err_bcp_read_addr_not_in_ddr(uint32_t mc);
void ddr_err_bcp_read_blocked_by_pas(uint32_t mc);
void ddr_err_bcp_write_addr_not_in_ddr(uint32_t mc);
void ddr_err_bcp_write_blocked_by_pas(uint32_t mc);
void ddr_err_bcp_chi_unsupported_opcode(uint32_t mc);

void ddr_err_inj_mrdp_parity_ue(uint32_t mc);

uint64_t ddr_err_inj_get_default_address();

acpi_einj_cmd_status_t ddr_error_injection_cb(ras_einj_info_t* einj_payload, void* ctx);
int get_syndrome_index(const char* name);