//
// Copyright (c) Microsoft Corporation.
//

/**
 * @file health_monitor_temporary_einj_structs.h
 * Temporary header file for supporting health monitoring until TFA shared header is available
 */
#pragma once

#include <cper.h>

#pragma pack(1)

// Memory locations for the vendor-defined error injection parameters
// These are valid only if flags == 0x2
#define PIONEER_EINJ_VENDOR_ERROR_PARAM1_ADDR (AP_EINJ_INST_BUFFER_BASE + RAS_EINJ_MEM_ADD_PARAM_OFFSET)
#define PIONEER_EINJ_VENDOR_ERROR_PARAM2_ADDR (AP_EINJ_INST_BUFFER_BASE + RAS_EINJ_MEM_ADD_RANGE_PARAM_OFFSET)
#define PIONEER_EINJ_VENDOR_ERROR_DOMAIN_ADDR PIONEER_EINJ_VENDOR_ERROR_PARAM1_ADDR // one byte indicating the error domain


/*
    The Whea error injection tool accepts commands in this format:
    > wheahctsa.exe /err <error_type> /param1 <flags> /param2 <param1> /param3 <param2> /param4 <param3>
    The /err argument is decimal-based
    The optional injection parameters are hex-based

    Standard error injections are defined by ACPI specification and can be found here:
    https://uefi.org/specs/ACPI/6.4/18_ACPI_Platform_Error_Interfaces/error-injection.html#
    Error Type definitions: 18.6.4

    The MSb of the /err argument should be set if the user wishes to use vendor-defined einj, for eg:
    > wheahctsa.exe /err 2147483648 /param1 0x2 /param2 <param1> /param3 <param2>

    Each error domain (Processor, Memory, PCIe, etc...) should define the param1 and param2 below,
    the only hard rules for the structure definition are for the two lowest bytes of param1
    byte0 (lowest byte) is ALWAYS the error domain, which is defined in ras.h according to acpi_error_domain_t
    byte1 is ALWAYS the error severity, which is defined in ras.h according to acpi_error_severity_t
*/
typedef struct _vendor_einj_header_t
{
    uint8_t error_domain;
    uint8_t error_severity;
} vendor_einj_header_t;


/*****************************************************************
 * ACPI_ERROR_DOMAIN_STD_MEMORY parameter definitions
 ****************************************************************/
typedef struct _std_mem_t
{
    uint64_t address;
    uint64_t address_range;
} std_mem_t;

/*****************************************************************
 * ACPI_ERROR_DOMAIN_DDR parameter definitions
 ****************************************************************/

typedef union _ddr_einj_params_t
{
    struct {
        uint32_t ddrss_index;
        uint32_t channel                :1;
        uint32_t rank                   :2;
        uint32_t bg                     :3;
        uint32_t bank                   :2;
        uint32_t row                    :16;
        uint32_t column                 :8;
    } params_ce, params_ue;

    struct {
        uint32_t ddrss_index;
        uint16_t phy_err;
        uint16_t reserved;
    } params_phy_err;

    struct {
        uint32_t ddrss_index;
        uint8_t channel;
        uint8_t reserved[3];
    } params_rd_retry_limit, params_wrrtrydat, params_wrrtrykbd, params_wrdat, params_temperature, params_rfm_alert;

    struct {
        uint32_t ddrss_index;
        uint8_t channel;
        uint8_t is_err:1;
        uint8_t is_fatal:1;
        uint8_t is_max_limit:1;
        uint8_t is_retry_limit:1;
        uint8_t reserved:4;
    } params_capar;

    struct {
        uint32_t ddrss_index;
        uint8_t chb_id;
        uint8_t buf_id;
        uint16_t reserved;
    } params_wrd, params_wrdat_parity, params_rddat, params_rdkbd;

    struct {
        uint32_t ddrss_index;
        uint8_t chb_id;
        uint8_t reserved[3];
    } params_rtrylst;

    struct {
        uint32_t ddrss_index;
        uint8_t channel;
        uint8_t ctrlupd:1;
        uint8_t lccmd:1;
        uint8_t ducmd:1;
        uint8_t swcmd:1;
        uint8_t reserved1:4;
        uint8_t reserved2[2];
    } params_cmd_err;
} ddr_einj_params_t;


// For Whea tool, /param2 and /param3 each take 8 bytes
typedef struct _vendor_ddr_einj_t
{
    // Total of 16 Bytes
    vendor_einj_header_t header;    // 2 Bytes
    uint8_t ddr_einj_type;          // 1 Bytes  See _ddr_einj_types_t
    uint8_t easy_ecc_mode;          // 1 Byte:  1=Inject & Trigger  2=Inject only
    uint8_t reserved[4];            // 4 Bytes
    ddr_einj_params_t einj_params;  // 8 Bytes
} vendor_mem_einj_t;

typedef enum _ddr_einj_types_t
{
    DDRSS_INJECT_CE,
    DDRSS_INJECT_UE,
    DDRSS_INJECT_CAPAR,
    DDRSS_INJECT_WRRTRYDAT,
    DDRSS_INJECT_WRDAT,
    DDRSS_INJECT_WRRTRYKBD,
    DDRSS_INJECT_WRDAT_PARITY,
    DDRSS_INJECT_RDDAT,
    DDRSS_INJECT_RDKBD,
    DDRSS_INJECT_RTRYLST,
    DDRSS_INJECT_CMD_ERR,
    DDRSS_INJECT_TEMPERATURE,
    DDRSS_INJECT_RFM_ALERT,
    DDRSS_INJECT_PHY_ERR,
    DDRSS_INJECT_WRD,
} ddr_einj_types;

/*****************************************************************
 * ACPI_ERROR_DOMAIN_MESH parameter definitions
 ****************************************************************/
typedef struct _vendor_mesh_einj_t
{
    // Total of 16 Bytes
    vendor_einj_header_t header;    // 2 Bytes
    uint8_t node_type;              // 1 Byte
    uint8_t node_id;                // 1 Byte
    uint16_t node_control_reg;      // 2 Bytes
    uint8_t reserved1[2];           // 2 Bytes
    uint32_t err_inj;               // 4 Bytes
    uint8_t byte_par_err_inj;       // 1 Byte
    uint8_t reserved2[3];           // 3 Bytes
} vendor_mesh_einj_t;

/*****************************************************************
 * ACPI_ERROR_DOMAIN_SCF_RAM parameter definitions
 ****************************************************************/
/*****************************************************************
 * ACPI_ERROR_DOMAIN_SECURE_RAM parameter definitions
 ****************************************************************/
/*****************************************************************
 * ACPI_ERROR_DOMAIN_NON_SECURE_RAM parameter definitions
 ****************************************************************/
/*****************************************************************
 * ACPI_ERROR_DOMAIN_BOOT_RAM parameter definitions
 ****************************************************************/

typedef struct _vendor_mcp_proc_einj_t
{
    // Total of 16 Bytes
    vendor_einj_header_t header;    // 2 Bytes
    uint8_t error_type;             // 1 Byte (refer to mcp_proc_err_type_t)
    uint8_t reserved[13];           // 13 Bytes

} vendor_mcp_proc_einj_t;

/*****************************************************************
 * ACPI_ERROR_DOMAIN_HSP_PROC parameter definitions
 ****************************************************************/

typedef enum _hsp_proc_err_type_t
{
    HSP_ERROR_TYPE_CRYPTO_CSM = 0,
    HSP_ERROR_TYPE_CRYPTO_INIT,
    HSP_ERROR_TYPE_CRYPTO_SS_MISMATCH,
    HSP_ERROR_TYPE_CRYPTO_AEB,
    HSP_ERROR_TYPE_CRYPTO_AES,
    HSP_ERROR_TYPE_CRYPTO_SHA,
    HSP_ERROR_TYPE_CRYPTO_PKA,
    HSP_ERROR_TYPE_CRYPTO_RNG,
    HSP_ERROR_TYPE_CRYPTO_CCS,
    HSP_ERROR_TYPE_UNCORR_ERR_PKAR1,    
    HSP_ERROR_TYPE_UNCORR_ERR_PKAR2,
    HSP_ERROR_TYPE_UNCORR_ERR_KEYSTR,
    HSP_ERROR_TYPE_UNCORR_ERR_SHAREDRAM,
    HSP_ERROR_TYPE_UNCORR_ERR_SPIRAM,
    HSP_ERROR_TYPE_UNCORR_ERR_SPDRAM,
    HSP_ERROR_TYPE_UNCORR_ERR_SPROM,
    HSP_ERROR_TYPE_CORR_ERR_PKAR1,    
    HSP_ERROR_TYPE_CORR_ERR_PKAR2,
    HSP_ERROR_TYPE_CORR_ERR_KEYSTR,
    HSP_ERROR_TYPE_CORR_ERR_SHAREDRAM,
    HSP_ERROR_TYPE_CORR_ERR_SPIRAM,
    HSP_ERROR_TYPE_CORR_ERR_SPDRAM,
    HSP_ERROR_TYPE_CORR_ERR_SPROM,
    HSP_ERROR_TYPE_ERR_ERASE_BUSY_PKAR1,    
    HSP_ERROR_TYPE_ERR_ERASE_BUSY_PKAR2,
    HSP_ERROR_TYPE_ERR_ERASE_BUSY_KEYSTR,
    HSP_ERROR_TYPE_ERR_ERASE_BUSY_SHAREDRAM,
    HSP_ERROR_TYPE_ERR_ERASE_BUSY_SPIRAM,
    HSP_ERROR_TYPE_ERR_ERASE_BUSY_SPDRAM,
    HSP_ERROR_TYPE_ERR_ERASE_BUSY_SPROM,
    HSP_ERROR_TYPE_CHKPT_EXPIRED,
    HSP_ERROR_TYPE_CHKPT_OVERFLOW,
    HSP_ERROR_TYPE_U1_DLOCK,
    HSP_ERROR_TYPE_U1_DECCERR_W,
    HSP_ERROR_TYPE_U1_DECCERR_R,
    HSP_ERROR_TYPE_AXI_R_ERR,
    HSP_ERROR_TYPE_AXI_W_ERR,
    HSP_ERROR_TYPE_SP_BUS_VIO_0,
    HSP_ERROR_TYPE_SP_BUS_VIO_1,
    HSP_ERROR_TYPE_ACC_VIO,
    HSP_ERROR_TYPE_DMB_ACC_VIO,
    HSP_ERROR_TYPE_SPIRAM_ACC_VIO,
    HSP_ERROR_TYPE_SPDRAM_ACC_VIO,
    HSP_ERROR_TYPE_SPROM_ACC_VIO,
    HSP_ERROR_TYPE_TIMER_INT_0,
    HSP_ERROR_TYPE_TIMER_INT_1,

    HSP_ERROR_TYPE_COUNT
} hsp_proc_err_type_t;

#if (HSP_ERROR_TYPE_COUNT > 256)
#error Too many HSP_PROC error types!
#endif

typedef struct _vendor_hsp_proc_einj_t
{
    // Total of 16 Bytes
    vendor_einj_header_t header;    // 2 Bytes
    uint8_t error_type;             // 1 Byte (refer to hsp_proc_err_type_t)
    uint8_t reserved[13];           // 13 Bytes

} vendor_hsp_proc_einj_t;

/*****************************************************************
 * ACPI_ERROR_DOMAIN_SCP_PROC parameter definitions
 ****************************************************************/
typedef struct _vendor_scp_proc_einj_t
{
    // Total of 16 Bytes
    vendor_einj_header_t header;    // 2 Bytes
    uint8_t error_type;             // 1 Byte (refer to scp_proc_err_type_t)
    uint8_t reserved[13];           // 13 Bytes

} vendor_scp_proc_einj_t;

typedef enum _ap_smmu_err_block_t
{
    AP_SMMU_ERROR_BLOCK_IOMACRO0_TCU = 0,
    AP_SMMU_ERROR_BLOCK_IOMACRO0_TBU_RD,
    AP_SMMU_ERROR_BLOCK_IOMACRO0_TBU_WR,
    AP_SMMU_ERROR_BLOCK_IOMACRO1_TCU,
    AP_SMMU_ERROR_BLOCK_IOMACRO1_TBU_RD,
    AP_SMMU_ERROR_BLOCK_IOMACRO1_TBU_WR,
    AP_SMMU_ERROR_BLOCK_IOMACRO2_TCU,
    AP_SMMU_ERROR_BLOCK_IOMACRO2_TBU_RD,
    AP_SMMU_ERROR_BLOCK_IOMACRO2_TBU_WR,
    AP_SMMU_ERROR_BLOCK_IOMACRO3_TCU,
    AP_SMMU_ERROR_BLOCK_IOMACRO3_TBU_RD,
    AP_SMMU_ERROR_BLOCK_IOMACRO3_TBU_WR,
    AP_SMMU_ERROR_BLOCK_IOMACRO4_TCU,
    AP_SMMU_ERROR_BLOCK_IOMACRO4_TBU_RD,
    AP_SMMU_ERROR_BLOCK_IOMACRO4_TBU_WR,
    AP_SMMU_ERROR_BLOCK_IOMACRO5_TCU,
    AP_SMMU_ERROR_BLOCK_IOMACRO5_TBU_RD,
    AP_SMMU_ERROR_BLOCK_IOMACRO5_TBU_WR,

    AP_SMMU_ERROR_BLOCK_COUNT
} ap_err_smmu_err_block_type_t;
#if (AP_SMMU_ERROR_BLOCK_COUNT > 256)
#error Too many AP SMMU error blocks!
#endif

typedef struct _vendor_ap_einj_t
{
    // Total of 16 Bytes
    vendor_einj_header_t header;    // 2 Bytes
    uint8_t error_type;             // 1 Byte (refer to ap_err_type_t)
    uint8_t error_block;            // 1 Byte (refer to ap_err_smmu_err_block_type_t)
    uint8_t reserved1[4];           // 4 Bytes
    uint64_t errins;                // 8 Byte
} vendor_ap_einj_t;

/*****************************************************************
 * ACPI_ERROR_DOMAIN_SMMU parameter definitions
 ****************************************************************/
/*****************************************************************
 * ACPI_ERROR_DOMAIN_PCIE parameter definitions
 ****************************************************************/
typedef struct
{
    uint32_t reserved8 : 8;
    uint32_t function : 3;
    uint32_t device : 5;
    uint32_t bus : 8;
    uint32_t segment : 8;
} pcie_sbdf_t;

typedef enum
{
    PCIE_ERR_TYPE_RX                    = 0,
    PCIE_ERR_TYPE_BAD_DLLP              = 1,
    PCIE_ERR_TYPE_CORRECTED_INTERNAL    = 2,
    PCIE_ERR_TYPE_POISON_TLP            = 3,
    PCIE_ERR_TYPE_COMPLETION_TIMEOUT    = 4,
    PCIE_ERR_TYPE_ECRC                  = 5,
    PCIE_ERR_TYPE_UNCORRECTED_INTERNAL  = 6,
    PCIE_ERR_TYPE_MALFORMED_TLP         = 7,
    PCIE_ERR_TYPE_UNSUPPORTED_REQUEST   = 8,
    PCIE_ERR_TYPE_REPLAY_TIMEOUT        = 9,
    PCIE_ERR_TYPE_RAS_DES_EINJ0_CRC     = 10,
    PCIE_ERR_TYPE_RAS_DES_EINJ1_SEQNUM  = 11,
    PCIE_ERR_TYPE_RAS_DES_EINJ2_DLLP    = 12,
    PCIE_ERR_TYPE_RAS_DES_EINJ3_SYMBOL  = 13,
    PCIE_ERR_TYPE_RAS_DES_EINJ4_FC      = 14,
    PCIE_ERR_TYPE_RAS_DES_EINJ5_SP_TLP  = 15
} PCIE_VENDOR_ERROR_TYPE;

typedef struct _vendor_pcie_einj_t
{
    // first parameter
    vendor_einj_header_t header;
    uint8_t vendor_error_type;
    uint8_t error_count;
    uint8_t error_location;
    uint8_t counter_selection;
    uint8_t counter_sel_region;
    uint8_t rasdp_en;

    // second parameter
    pcie_sbdf_t sbdf;
    uint8_t trigger_en;
    uint8_t trigger_method;
    uint8_t reserved[2];
} vendor_pcie_einj_t;

/*****************************************************************
 * ACPI_ERROR_DOMAIN_GIC parameter definitions
 ****************************************************************/
typedef struct _vendor_gic_einj_t
{
    // Total of 16 Bytes
    vendor_einj_header_t header;    // 2 Bytes, Note: error_severity is not used
    uint8_t reserved1[2];           // 2 Bytes
    uint32_t offset;                // 4 Bytes
    uint64_t errins;                // 8 Byte
} vendor_gic_einj_t;

/*****************************************************************
 * Injection structure in memory
 ****************************************************************/
typedef union _msft_einj_params_t
{
    vendor_einj_header_t vendor_header;
    std_mem_t std_mem;
    vendor_pcie_einj_t pcie;
    vendor_mesh_einj_t mesh;
    vendor_gic_einj_t gic;
    vendor_ap_einj_t ap;
    vendor_scp_proc_einj_t scp_proc;
    vendor_mcp_proc_einj_t mcp_proc;
    vendor_mem_einj_t mem;
    vendor_hsp_proc_einj_t hsp_proc;
    //vendor_proc_einj_t proc;
    uint64_t as_uint64[2];
} msft_einj_params_t;

typedef struct {
    uint32_t version;
    uint16_t component_group;
    uint16_t component_type;
    uint16_t component_instance;
    union {
        uint16_t value;
        struct {
            uint8_t status;
            enum operation_t : uint8_t {
                ENUM_GET,
                ENUM_SET,
                ADDRESS_GET,
                ADDRESS_SET
            } operation;
        };
    } status_operation;
    union {
        uint64_t error_parameters[2];
        struct {
            uint16_t error_type;
            uint16_t severity;
            uint32_t reserved;
            union {
                uint64_t *address_64;
                uint32_t *address_32;
            };
        };
    } param_type;
    union {
        uint64_t error_values;
        uint64_t data_64;
        uint32_t data_32;
    } value_type;
} ras_einj_info_t;

typedef struct _ras_einj_t
{
    ras_einj_info_t info;
    uint64_t get_trigger_table;
    uint64_t check_busy;
    uint64_t cmd_status;
    uint8_t end_operation[200];
    uint8_t trigger_table[12];
    uint32_t trigger_table_size;
} ras_einj_t;
#pragma pack()