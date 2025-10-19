//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_acpi_smbios_tables.c
 *
 * This file will create BDAT and SMBIOS Type 16/17 tables for UEFI
 */

/*------------- Includes -----------------*/

#include "ddr_manager_i.h"
#include "memory_map/ddrss_reserved_regions.h"

#include <atu_lib.h>
#include <bdat_schema.h>
#include <bug_check.h>
#include <ddr_i3c.h>
#include <ddrss.h>
#include <ddrss_dimm.h>
#include <ddrss_lib.h>
#include <dw_i3c.h>
#include <i3c_controller.h>
#include <icc_platform_defines.h>
#include <idhw.h>
#include <idsw_kng.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_exp_spi_synchronize_dies.h>
#include <smbios_structs.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define BDAT_SUPPORTED_MSFT_SCHEMAS     0x1
#define BDAT_RESERVATION_SIZE           (BDAT_DATA_END - BDAT_DATA_START)
#define SMBIOS_HANDOFF_RESERVATION_SIZE (SMBIOS_HANDOFF_RESERVATION_END - SMBIOS_HANDOFF_RESERVATION_BASE)

// Used for SMBIOS TYPE17 table
#define PARTNUM_NUMBYTES          (29) // Does not include NULL byte
#define STRING_SIZE               (PARTNUM_NUMBYTES + 1)
#define MAX_NUM_DIMM_VENDOR_CHARS (16)

// Offsets as defined in JEDEC spec for these fields in the 1KB SPD array
#define START_OF_VENDOR_INFO           (512)
#define SERIALNUM_SPD_OFFSET           (START_OF_VENDOR_INFO)
#define PARTNUM_SPD_OFFSET             (521)
#define MODULE_MANUFACTURER_SPD_OFFSET (SERIALNUM_SPD_OFFSET + 1)

// clang-format off
// bdat_valid_bytes represents 1 bit per 'valid' byte of SPD data
static const uint8_t bdat_valid_bytes[1024/8] = {
    /* 0-63 Base Configuration and DRAM Parameters */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 64-127 Base Configuration and DRAM Parameters */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 128-191 Reserved for future use */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 192-239 Common Module Parameters */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 240-255 Standard Module Parameters */
    0xFF, 0xFF,
    /* 256-319 Standard Module Parameters */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 320-383 Standard Module Parameters */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 384-447 Standard Module Parameters */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 448-509 (62 Bytes) Reserved for future use
       510-511 (2 Bytes) CRC for bytes 0-509 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
    /* 512-575 Manufacturing Information */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 576-639 Manufacturing Information */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 640-703 End User Programmable */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 704-767 End User Programmable */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 768-831 End User Programmable */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 832-895 End User Programmable */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 896-959 End User Programmable */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* 960-1023 End User Programmable */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
// clang-format on

// Representation of the silk screen DIMM slot identifiers on the board
static const char DIMM_SLOT_ID[12] = {'D', 'K', 'F', 'M', 'E', 'L', 'G', 'A', 'H', 'B', 'J', 'C'};

/*------------- Typedefs -----------------*/
typedef union _guid_bytes_t
{
    struct
    {
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        uint8_t Data4[8];
    } Guid;

    uint32_t AsUint32[4];
    uint8_t AsByte[16];
} guid_bytes_t;

typedef enum
{
    MICRON_IDX = 0,
    SK_HYNIX_IDX,
    SAMSUNG_IDX,
    UNKNOWN_IDX,
    NUM_DIMM_VENDORS,
} DIMM_VENDOR_IDX_t;

/*-------- Function Prototypes -----------*/
static uint16_t CalculateRemoteCheckSum16(uint32_t BufferAddr, uint32_t Length);
const char* dimm_vendor_from_id(uint8_t id_byte);

/*-- Declarations (Statics and globals) --*/
static const char dimm_vendor_str[NUM_DIMM_VENDORS][MAX_NUM_DIMM_VENDOR_CHARS] = {{"Micron\0"},
                                                                                  {"SK Hynix\0"},
                                                                                  {"Samsung\0"},
                                                                                  {"Unknown\0"}};

/*------------- Functions ----------------*/
void ddr_create_bdat(void)
{
    // Get DDR training data from ddrss silicon libs
    // Create ACPI BDAT
    // Copy to reserved DDR region specified by shared header file
    atu_map_entry_t bdat_mem_atu_map_struct;
    KNG_DIE_ID die_num = idsw_get_die_id();
    uint16_t data_len;
    uint32_t mc;
    uint16_t dimm_global_idx;
    uint8_t Bios_data_sign[] = {'B', 'D', 'A', 'T', 'H', 'E', 'A', 'D'};
    atu_entry_attr_t bdat_test_atu_root_attr = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_NS};
    ddrss_cfg_knobs_t ddrss_prd_cfg_knobs = {0};
    int sts = SILIBS_SUCCESS;

    sts = ddrss_get_config(&ddrss_prd_cfg_knobs);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);

    ddrss_phy_training_margin_t ddrss_phy_training_margin[DDRSS_SS_NUM_PER_DIE];
    uint32_t fw_ver = ddrss_get_fw_version();

    bdat_mem_atu_map_struct.ap_base_address = BDAT_DATA_START;
    bdat_mem_atu_map_struct.mscp_start_address = 0;
    bdat_mem_atu_map_struct.mscp_end_address = BDAT_RESERVATION_SIZE - 1;
    bdat_mem_atu_map_struct.attribute.as_uint32 = bdat_test_atu_root_attr.as_uint32;

    sts = atu_map(ATU_ID_MSCP, &bdat_mem_atu_map_struct);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);

    printf("Create DDR BDAT Table...\n");

    // Get the new training per-lane margin data
    ddrss_phy_training_dq_margin_t* dq_margin = ddrss_get_training_margin_base();
    BUG_ASSERT_PARAM(dq_margin != NULL, dq_margin, 0);

    if (die_num == DIE_0)
    {
        // clear memory first - all 32KB
        for (int i = 0; i < (BDAT_RESERVATION_SIZE / 4); i++)
        {
            MMIO_WRITE32((bdat_mem_atu_map_struct.mscp_start_address + (4 * i)), 0);
        }
    }

    if (!idhw_is_single_die_boot_en())
    {
        mscp_exp_spi_sync_point_t d2d_ddr_sync_point;
        d2d_ddr_sync_point.local_write_addr = SCP_EXP_D2D_SYNC_DDR_BDAT_BASE;
        d2d_ddr_sync_point.remote_write_addr = SCP_EXP_D2D_SYNC_DDR_BDAT_BASE + sizeof(uint32_t);
        d2d_ddr_sync_point.value = RMSS_D2D_DDR_BDAT_BEGIN_SYNC_POINT;

        if (mscp_exp_spi_synchronize_dies(d2d_ddr_sync_point, die_num) != SILIBS_SUCCESS)
        {
            // Raise error
            printf("Error Syncing dies\n");
        }
    }

    if (die_num == DIE_0)
    {
        // create BDAT header
        for (uint32_t i = 0; i < (sizeof(Bios_data_sign)); i++)
        {
            MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                            offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatHeader.BiosDataSignature[i]),
                        Bios_data_sign[i]);
        }
        MMIO_WRITE32(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatHeader.BiosDataStructSize),
                     sizeof(BDAT_DATA_STRUCTURE_MSFT_5));
        // crc calculated at last
        MMIO_WRITE16(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatHeader.Reserved),
                     0);
        MMIO_WRITE16(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatHeader.PrimaryVersion),
                     BDAT_PRIMARY_VER);
        MMIO_WRITE16(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatHeader.SecondaryVersion),
                     BDAT_SECONDARY_VER);
        MMIO_WRITE32(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatHeader.OemOffset),
                     0);
        MMIO_WRITE32(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatHeader.Reserved1),
                     0);
        MMIO_WRITE32(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatHeader.Reserved2),
                     0);

        // create BDAT schema list
        MMIO_WRITE16(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatSchemas.SchemaListLength),
                     BDAT_SUPPORTED_MSFT_SCHEMAS);
        MMIO_WRITE16(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatSchemas.Reserved),
                     0);
        MMIO_WRITE16(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatSchemas.Year),
                     0);
        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatSchemas.Month),
                    0);
        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatSchemas.Day),
                    0);
        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatSchemas.Hour),
                    0);
        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatSchemas.Minute),
                    0);
        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatSchemas.Second),
                    0);
        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatSchemas.Reserved1),
                    0);
        MMIO_WRITE32(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatSchemas.Schemas[0]),
                     sizeof(BDAT_STRUCTURE_MSFT_4));

        /* --------------------------- (Top level) BDAT_DATA_STRUCTURE_MSFT_5 --------------------------- */
        uint32_t addr = bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.refCodeRevision);
        MMIO_WRITE8(addr, (uint8_t)(fw_ver & 0xFF));
        MMIO_WRITE8(addr + 1, (uint8_t)((fw_ver >> 8) & 0xFF));
        MMIO_WRITE8(addr + 2, (uint8_t)((fw_ver >> 16) & 0xFF));
        MMIO_WRITE8(addr + 3, (uint8_t)((fw_ver >> 24) & 0xFF));

        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.maxNode),
                    MAX_SOCKET);
        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.maxCh),
                    MAX_CH);
        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.maxDimm),
                    MAX_DIMM);
        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.maxRankDimm),
                    MAX_RANK_DIMM);
        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.maxSubchannelRank),
                    MAX_SUBCHANNEL);
    } // end of DIE_0 ONLY

    /* bdat_SOCKET_STRUCTURE_MSFT_4 */
    for (size_t socket_idx = 0; socket_idx < MAX_SOCKET; socket_idx++)
    {
        if (die_num == DIE_0)
        {
            uint32_t addr = bdat_mem_atu_map_struct.mscp_start_address +
                            offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.socketList[socket_idx].PiStepUnit);
            uint16_t data = (uint16_t)ddrss_get_latency_step_unit();
            MMIO_WRITE8(addr, (uint8_t)(data & 0xFF));
            MMIO_WRITE8(addr + 1, (uint8_t)((data >> 8) & 0xFF));

            addr = bdat_mem_atu_map_struct.mscp_start_address +
                   offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.socketList[socket_idx].RxVrefStepUnit);
            data = (uint16_t)ddrss_get_rx_vref_step_unit();
            MMIO_WRITE8(addr, (uint8_t)(data & 0xFF));
            MMIO_WRITE8(addr + 1, (uint8_t)((data >> 8) & 0xFF));

            addr = bdat_mem_atu_map_struct.mscp_start_address +
                   offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.socketList[socket_idx].TxVrefStepUnit);
            data = (uint16_t)ddrss_get_device_vref_step_unit();
            MMIO_WRITE8(addr, (uint8_t)(data & 0xFF));
            MMIO_WRITE8(addr + 1, (uint8_t)((data >> 8) & 0xFF));

            addr = bdat_mem_atu_map_struct.mscp_start_address +
                   offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.socketList[socket_idx].CSVrefStepUnit);
            data = (uint16_t)ddrss_get_device_vref_step_unit();
            MMIO_WRITE8(addr, (uint8_t)(data & 0xFF));
            MMIO_WRITE8(addr + 1, (uint8_t)((data >> 8) & 0xFF));

            addr = bdat_mem_atu_map_struct.mscp_start_address +
                   offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.socketList[socket_idx].CAVrefStepUnit);
            data = (uint16_t)ddrss_get_device_vref_step_unit();
            MMIO_WRITE8(addr, (uint8_t)(data & 0xFF));
            MMIO_WRITE8(addr + 1, (uint8_t)((data >> 8) & 0xFF));

            addr = bdat_mem_atu_map_struct.mscp_start_address +
                   offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.socketList[socket_idx].DeviceVrefStepUnit);
            data = (uint16_t)ddrss_get_device_vref_step_unit();
            MMIO_WRITE8(addr, (uint8_t)(data & 0xFF));
            MMIO_WRITE8(addr + 1, (uint8_t)((data >> 8) & 0xFF));

            addr = bdat_mem_atu_map_struct.mscp_start_address +
                   offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.socketList[socket_idx].ddrVoltage);
            data = (uint16_t)ddrss_get_vdd_voltage();
            MMIO_WRITE8(addr, (uint8_t)(data & 0xFF));
            MMIO_WRITE8(addr + 1, (uint8_t)((data >> 8) & 0xFF));

            addr = bdat_mem_atu_map_struct.mscp_start_address +
                   offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.socketList[socket_idx].ddrFreq);
            data = (uint16_t)ddrss_get_dimm_speed_mts() / 2;
            MMIO_WRITE8(addr, (uint8_t)(data & 0xFF));
            MMIO_WRITE8(addr + 1, (uint8_t)((data >> 8) & 0xFF));
        } // end of DIE_0 ONLY

        // Both DIEs iterate over thier respective 6 DIMMs (dimm_global_idx)
        for (uint16_t dimm_local_idx = 0; dimm_local_idx < DDRSS_SS_NUM_PER_DIE; dimm_local_idx++)
        {
            dimm_global_idx = (die_num * 6) + dimm_local_idx; // 6 dimm per die: die0- 0 to 5, die1- 6 to 11
            mc = dimm_global_idx * 2;

            bool this_dimm_is_present = dimm_is_present(dimm_local_idx);
            if (this_dimm_is_present)
            {
                /* BDAT_CHANNEL_STRUCTURE_MSFT_5 */
                MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                                offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                         bdat_mem_data.socketList[socket_idx].channelList[dimm_global_idx].chEnabled),
                            1);
                /* BDAT_SPD_STRUCTURE_MSFT_4 */
                // Write 'valid' bytes based on JEDEC JESD400-5 DDR5 SPD Contents 1.0 Beta 0
                // bdat_valid_bytes is an array where each bit set represents 1 valid byte of SPD data.
                // This array is 128 Bytes long
                for (size_t validBytes_idx = 0; validBytes_idx < (MAX_SPD_BYTE_1024 / 8); validBytes_idx++)
                {
                    MMIO_WRITE8(
                        bdat_mem_atu_map_struct.mscp_start_address +
                            offsetof(
                                BDAT_DATA_STRUCTURE_MSFT_5,
                                bdat_mem_data.socketList[socket_idx].channelList[dimm_global_idx].spdBytes.valid[validBytes_idx]),
                        bdat_valid_bytes[validBytes_idx]);
                }

                // Copy 1KB of 'SPD/nvm' data from buffer to array in BDAT DDR region
                uint8_t spd_buff_1k[MAX_SPD_BYTE_1024] = {0};
                uint16_t data_offset = 0;
                i3c_cmd_t s_i3c_cmd = {0};
                data_len = 0;
                if (ddr_i3c_interface_read_spd_nvm_data(&s_i3c_cmd, dimm_global_idx, spd_buff_1k, &data_len, &data_offset, SIZE_1024_BYTES) !=
                    SILIBS_SUCCESS)
                {
                    printf("Error reading DIE=%d global DIMM=%d SPD", die_num, dimm_global_idx);
                }

                for (size_t spdData_idx = 0; spdData_idx < (MAX_SPD_BYTE_1024); spdData_idx++)
                {
                    MMIO_WRITE8(
                        bdat_mem_atu_map_struct.mscp_start_address +
                            offsetof(
                                BDAT_DATA_STRUCTURE_MSFT_5,
                                bdat_mem_data.socketList[socket_idx].channelList[dimm_global_idx].spdBytes.spdData[spdData_idx]),
                        spd_buff_1k[spdData_idx]);
                }

                /* BDAT_RANK_STRUCTURE_MSFT_5 */
                // read ddrs margin data per dimm
                if (ddrss_phy_get_training_margin(mc, &ddrss_phy_training_margin[dimm_local_idx]) != SILIBS_SUCCESS)
                {
                    printf("Error reading DIE=%d global DIMM=%d margin data\n", die_num, dimm_global_idx);
                }

                for (size_t rank_idx = 0; rank_idx < ddrss_prd_cfg_knobs.dimm_rank; rank_idx++)
                {
                    MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                                    offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                             bdat_mem_data.socketList[socket_idx]
                                                 .channelList[dimm_global_idx]
                                                 .rankList[rank_idx]
                                                 .rankEnabled),
                                1);

                    /* BDAT_SUBCHANNEL_STRUCTURE_MSFT_4 */
                    for (size_t subchan_idx = 0; subchan_idx < MAX_SUBCHANNEL; subchan_idx++)
                    {
                        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                                 bdat_mem_data.socketList[socket_idx]
                                                     .channelList[dimm_global_idx]
                                                     .rankList[rank_idx]
                                                     .subchannelList[subchan_idx]
                                                     .CsDlyMargin),
                                    ddrss_phy_training_margin[dimm_local_idx].margin[subchan_idx][rank_idx].cs_dly_margin);
                        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                                 bdat_mem_data.socketList[socket_idx]
                                                     .channelList[dimm_global_idx]
                                                     .rankList[rank_idx]
                                                     .subchannelList[subchan_idx]
                                                     .CsVrefMargin),
                                    ddrss_phy_training_margin[dimm_local_idx].margin[subchan_idx][rank_idx].cs_vref_margin);
                        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                                 bdat_mem_data.socketList[socket_idx]
                                                     .channelList[dimm_global_idx]
                                                     .rankList[rank_idx]
                                                     .subchannelList[subchan_idx]
                                                     .CaDlyMargin),
                                    ddrss_phy_training_margin[dimm_local_idx].margin[subchan_idx][rank_idx].ca_dly_margin);
                        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                                 bdat_mem_data.socketList[socket_idx]
                                                     .channelList[dimm_global_idx]
                                                     .rankList[rank_idx]
                                                     .subchannelList[subchan_idx]
                                                     .CaVrefMargin),
                                    ddrss_phy_training_margin[dimm_local_idx].margin[subchan_idx][rank_idx].ca_vref_margin);
                        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                                 bdat_mem_data.socketList[socket_idx]
                                                     .channelList[dimm_global_idx]
                                                     .rankList[rank_idx]
                                                     .subchannelList[subchan_idx]
                                                     .RxClkDlyMargin),
                                    ddrss_phy_training_margin[dimm_local_idx].margin[subchan_idx][rank_idx].rx_clk_dly_margin);
                        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                                 bdat_mem_data.socketList[socket_idx]
                                                     .channelList[dimm_global_idx]
                                                     .rankList[rank_idx]
                                                     .subchannelList[subchan_idx]
                                                     .VrefDacMargin),
                                    ddrss_phy_training_margin[dimm_local_idx].margin[subchan_idx][rank_idx].vref_dac_margin);
                        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                                 bdat_mem_data.socketList[socket_idx]
                                                     .channelList[dimm_global_idx]
                                                     .rankList[rank_idx]
                                                     .subchannelList[subchan_idx]
                                                     .TxDqDlyMargin),
                                    ddrss_phy_training_margin[dimm_local_idx].margin[subchan_idx][rank_idx].tx_dq_dly_margin);
                        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                                 bdat_mem_data.socketList[socket_idx]
                                                     .channelList[dimm_global_idx]
                                                     .rankList[rank_idx]
                                                     .subchannelList[subchan_idx]
                                                     .DeviceVrefMargin),
                                    ddrss_phy_training_margin[dimm_local_idx].margin[subchan_idx][rank_idx].device_vref_margin);
                        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                                 bdat_mem_data.socketList[socket_idx]
                                                     .channelList[dimm_global_idx]
                                                     .rankList[rank_idx]
                                                     .subchannelList[subchan_idx]
                                                     .QCsDlyMargin),
                                    ddrss_phy_training_margin[dimm_local_idx].margin[subchan_idx][rank_idx].q_cs_dly_margin);
                        MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                                 bdat_mem_data.socketList[socket_idx]
                                                     .channelList[dimm_global_idx]
                                                     .rankList[rank_idx]
                                                     .subchannelList[subchan_idx]
                                                     .QCaDlyMargin),
                                    ddrss_phy_training_margin[dimm_local_idx].margin[subchan_idx][rank_idx].q_ca_dly_margin);
                    }

                    /* New DQ margin data (big) */
                    for (size_t dq_lane_margin_idx = 0; dq_lane_margin_idx < MAX_DDRSS_DQ_LANE_MARGIN_DBYTE_NUM;
                         dq_lane_margin_idx++)
                    {
                        for (size_t tx_rx_lane_idx = 0; tx_rx_lane_idx < MAX_DDRSS_DQ_LANE_MARGIN_TX_RX_LANE_NUM;
                             tx_rx_lane_idx++)
                        {
                            uint32_t addr = bdat_mem_atu_map_struct.mscp_start_address +
                                            offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                                     bdat_mem_data.socketList[socket_idx]
                                                         .channelList[dimm_global_idx]
                                                         .rankList[rank_idx]
                                                         .marginList[dq_lane_margin_idx][tx_rx_lane_idx]
                                                         .dword[0]);
                            uint32_t data32 = (dq_margin[dimm_local_idx])
                                                  .margin[rank_idx][dq_lane_margin_idx][tx_rx_lane_idx]
                                                  .dword[0];
                            MMIO_WRITE8(addr, (uint8_t)data32 & 0xFF);
                            MMIO_WRITE8(addr + 1, (uint8_t)((data32 >> 8) & 0xFF));
                            MMIO_WRITE8(addr + 2, (uint8_t)((data32 >> 16) & 0xFF));
                            MMIO_WRITE8(addr + 3, (uint8_t)((data32 >> 24) & 0xFF));

                            addr = bdat_mem_atu_map_struct.mscp_start_address +
                                   offsetof(BDAT_DATA_STRUCTURE_MSFT_5,
                                            bdat_mem_data.socketList[socket_idx]
                                                .channelList[dimm_global_idx]
                                                .rankList[rank_idx]
                                                .marginList[dq_lane_margin_idx][tx_rx_lane_idx]
                                                .dword[1]);
                            data32 = (dq_margin[dimm_local_idx])
                                         .margin[rank_idx][dq_lane_margin_idx][tx_rx_lane_idx]
                                         .dword[1];
                            MMIO_WRITE8(addr, (uint8_t)data32 & 0xFF);
                            MMIO_WRITE8(addr + 1, (uint8_t)((data32 >> 8) & 0xFF));
                            MMIO_WRITE8(addr + 2, (uint8_t)((data32 >> 16) & 0xFF));
                            MMIO_WRITE8(addr + 3, (uint8_t)((data32 >> 24) & 0xFF));
                        }
                    }
                }
            }
            else
            {
                printf("BDAT: Skipping global DIMM %d - not enabled\n", dimm_global_idx);
            }
        }
    }

    /* BDAT_SCHEMA_HEADER_STRUCTURE_MSFT_5 */
    guid_bytes_t BDAT_GUID;
    BDAT_GUID.Guid.Data1 = (guid_bytes_t){BDAT_CUSTOM_GUID_MSFT_5}.Guid.Data1;
    BDAT_GUID.Guid.Data2 = (guid_bytes_t){BDAT_CUSTOM_GUID_MSFT_5}.Guid.Data2;
    BDAT_GUID.Guid.Data3 = (guid_bytes_t){BDAT_CUSTOM_GUID_MSFT_5}.Guid.Data3;
    BDAT_GUID.Guid.Data4[0] = (guid_bytes_t){BDAT_CUSTOM_GUID_MSFT_5}.Guid.Data4[0];
    BDAT_GUID.Guid.Data4[1] = (guid_bytes_t){BDAT_CUSTOM_GUID_MSFT_5}.Guid.Data4[1];
    BDAT_GUID.Guid.Data4[2] = (guid_bytes_t){BDAT_CUSTOM_GUID_MSFT_5}.Guid.Data4[2];
    BDAT_GUID.Guid.Data4[3] = (guid_bytes_t){BDAT_CUSTOM_GUID_MSFT_5}.Guid.Data4[3];
    BDAT_GUID.Guid.Data4[4] = (guid_bytes_t){BDAT_CUSTOM_GUID_MSFT_5}.Guid.Data4[4];
    BDAT_GUID.Guid.Data4[5] = (guid_bytes_t){BDAT_CUSTOM_GUID_MSFT_5}.Guid.Data4[5];
    BDAT_GUID.Guid.Data4[6] = (guid_bytes_t){BDAT_CUSTOM_GUID_MSFT_5}.Guid.Data4[6];
    BDAT_GUID.Guid.Data4[7] = (guid_bytes_t){BDAT_CUSTOM_GUID_MSFT_5}.Guid.Data4[7];

    const int GUID_SIZE_BYTES = 16;

    if (die_num == DIE_0)
    {
        for (int byte_idx = 0; byte_idx < GUID_SIZE_BYTES; byte_idx++)
        {
            MMIO_WRITE8(bdat_mem_atu_map_struct.mscp_start_address +
                            offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.schemaHeader.Guid) + byte_idx,
                        BDAT_GUID.AsByte[byte_idx]);
        }

        uint32_t addr = bdat_mem_atu_map_struct.mscp_start_address +
                        offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.schemaHeader.DataSize);
        MMIO_WRITE8(addr, (uint8_t)(sizeof(BDAT_DATA_STRUCTURE_MSFT_5) & 0xFF));
        MMIO_WRITE8(addr + 1, (uint8_t)((sizeof(BDAT_DATA_STRUCTURE_MSFT_5) >> 8) & 0xFF));
        MMIO_WRITE8(addr + 2, (uint8_t)((sizeof(BDAT_DATA_STRUCTURE_MSFT_5) >> 16) & 0xFF));
        MMIO_WRITE8(addr + 3, (uint8_t)((sizeof(BDAT_DATA_STRUCTURE_MSFT_5) >> 24) & 0xFF));

        // Calculate checksum with schemaHeader.Crc16 as 0 & then overwrite
        addr = bdat_mem_atu_map_struct.mscp_start_address +
               offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.schemaHeader.Crc16);
        MMIO_WRITE8(addr, (uint8_t)(0 & 0xFF));
        MMIO_WRITE8(addr + 1, (uint8_t)((0 >> 8) & 0xFF));

        addr = bdat_mem_atu_map_struct.mscp_start_address +
               offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatHeader.Crc16);
        MMIO_WRITE8(addr, (uint8_t)(0 & 0xFF));
        MMIO_WRITE8(addr + 1, (uint8_t)((0 >> 8) & 0xFF));
    } // end of DIE_0 ONLY

    if (!idhw_is_single_die_boot_en())
    {
        mscp_exp_spi_sync_point_t d2d_ddr_sync_point;
        d2d_ddr_sync_point.local_write_addr = SCP_EXP_D2D_SYNC_DDR_BDAT_CRC_BASE;
        d2d_ddr_sync_point.remote_write_addr = SCP_EXP_D2D_SYNC_DDR_BDAT_CRC_BASE + sizeof(uint32_t);
        d2d_ddr_sync_point.value = RMSS_D2D_DDR_BDAT_CRC_READY_SYNC_POINT;

        if (mscp_exp_spi_synchronize_dies(d2d_ddr_sync_point, die_num) != SILIBS_SUCCESS)
        {
            // Raise error
            printf("Error Syncing dies\n");
        }
    }

    if (die_num == DIE_0)
    {
        uint16_t bdat_crc = CalculateRemoteCheckSum16(bdat_mem_atu_map_struct.mscp_start_address,
                                                      sizeof(BDAT_DATA_STRUCTURE_MSFT_5));
        MMIO_WRITE16(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_mem_data.schemaHeader.Crc16),
                     bdat_crc);
        MMIO_WRITE16(bdat_mem_atu_map_struct.mscp_start_address +
                         offsetof(BDAT_DATA_STRUCTURE_MSFT_5, bdat_struct.BdatHeader.Crc16),
                     bdat_crc);
    }

    sts = atu_unmap(ATU_ID_MSCP, &bdat_mem_atu_map_struct);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
    printf("BDAT DONE\n");
}

// Search from the end back to the first non-null or non-space character and then insert a NULL byte.
// Return total number of characters including NULL
static size_t trim_trailing_whitespace(char* tempString, int max_chars)
{
    int num_chars = 0;
    int i = 0;

    // Find the last non-null or non-space character
    for (i = max_chars - 1; i >= 0; i--)
    {
        if (tempString[i] != '\0' && tempString[i] != ' ')
        {
            break;
        }
    }

    // Insert NULL byte
    tempString[i + 1] = '\0';

    // Return total number of characters including NULL
    num_chars = i + 2;

    return num_chars;
}

static void ddr_wait_for_smbios_sync_point(void)
{
    int sts = SILIBS_SUCCESS;

    if (!idhw_is_single_die_boot_en())
    {
        mscp_exp_spi_sync_point_t d2d_ddr_sync_point;
        d2d_ddr_sync_point.local_write_addr = SCP_EXP_D2D_SYNC_DDR_SMBIOS16_BASE;
        d2d_ddr_sync_point.remote_write_addr = SCP_EXP_D2D_SYNC_DDR_SMBIOS16_BASE + sizeof(uint32_t);
        d2d_ddr_sync_point.value = RMSS_D2D_DDR_SMBIOS16_DONE_SYNC_POINT;

        KNG_DIE_ID die_num = idsw_get_die_id();
        sts = mscp_exp_spi_synchronize_dies(d2d_ddr_sync_point, die_num);
        BUG_ASSERT(sts == SILIBS_SUCCESS);
    }
}

static uint32_t ddr_create_smbios_type_16(uint32_t smbios_next_addr)
{
    uint32_t dimm_capacity_gb = get_i3c_dimm_cap_in_gb();
    uint32_t dimm_capacity_mb = dimm_capacity_gb * 1024;
    uint32_t dimm_capacity_kb = dimm_capacity_mb * 1024;

    const uint32_t EXTENDED_DIMM_CAP_LIMIT_KB_TYPE16 = 0x80000000;

    SMBIOS_PHYS_MEM_ARRAY_16 smb_table16 = {0};
    // --------------------------- Type 16 table ---------------------------
    smb_table16.Type = 16;
    smb_table16.Length = 0x17; // Length of the structure, 0Fh for version 2.1, 17h for version 2.7 and later
    smb_table16.Handle = 0;
    smb_table16.Location = 0x3; // System board or motherboard, refer to Table 72 of SMBIOS Ref Spec (DSP0134_3.8.0)
    smb_table16.Use = 0x3; // System memory, refer to Table 73 of SMBIOS Ref Spec (DSP0134_3.8.0)
    smb_table16.MemoryErrorCorrection = 0x3; // None, refer to Table 74 of SMBIOS Ref Spec (DSP0134_3.8.0)

    /* Maximum memory capacity, in kilobytes, for this array
    If the capacity is not represented in this field, then this
    field contains 8000 0000h and the Extended
    Maximum Capacity field should be used. Values 2 TB
    (8000 0000h) or greater must be represented in the
    Extended Maximum Capacity field.

    MaximumCapacity expressed in kB
    ExtendedMaximumCapacity expressed in B
    */
    if (dimm_capacity_kb >= EXTENDED_DIMM_CAP_LIMIT_KB_TYPE16)
    {
        smb_table16.MaximumCapacity = EXTENDED_DIMM_CAP_LIMIT_KB_TYPE16;
        smb_table16.ExtendedMaximumCapacity = (uint64_t)dimm_capacity_kb * 1024; // In Bytes
    }
    else
    {
        smb_table16.MaximumCapacity = dimm_capacity_kb;
        smb_table16.ExtendedMaximumCapacity = 0;
    }

    smb_table16.MemoryErrorInformationHandle = 0;
    smb_table16.NumberOfMemoryDevices = NUM_DIMM_PER_DIE;

    // Copy Type16 table to memory
    uint8_t temp_buffer[sizeof(smb_table16)];
    memcpy(temp_buffer, &smb_table16, sizeof(smb_table16));
    for (size_t byte_idx = 0; byte_idx < sizeof(smb_table16); byte_idx++)
    {
        MMIO_WRITE8(smbios_next_addr++, temp_buffer[byte_idx]);
    }

    // Append double NULL to signify end of (empty) string section for Type 16 table
    MMIO_WRITE8(smbios_next_addr++, 0);
    MMIO_WRITE8(smbios_next_addr++, 0);

    // Align to next 32bit addr
    if (!FPFW_IS_ALIGNED(smbios_next_addr, sizeof(uint32_t)))
    {
        smbios_next_addr = FPFW_ALIGN_SIZE_TO_4_BYTES(smbios_next_addr);
    }

    printf("SMBIOS 16 DONE\n");
    return smbios_next_addr;
}

static uint32_t ddr_create_smbios_type_17(uint32_t smbios_next_addr)
{
    char tempString[STRING_SIZE] = {0};
    SMBIOS_MEM_DEVICE_17 smb_table17 = {0};
    ddrss_cfg_knobs_t ddrss_prd_cfg_knobs = {0};

    uint32_t dimm_capacity_gb = get_i3c_dimm_cap_in_gb();
    uint32_t dimm_capacity_mb = dimm_capacity_gb * 1024;
    uint32_t dimm_capacity_kb = dimm_capacity_mb * 1024;

    const uint16_t JEDEC_MICROSOFT_ID = 0xD5;
    const uint32_t EXTENDED_DIMM_CAP_LIMIT_MB_TYPE17 = 0x7FFF;

    // --------------------------- Type 17 tables ---------------------------
    uint32_t this_dies_smbios_next_addr = smbios_next_addr;
    uint16_t dimm_global_idx;
    uint8_t spd_buff[SIZE_64_BYTES] = {0};
    uint16_t data_offset = START_OF_VENDOR_INFO;
    i3c_cmd_t s_i3c_cmd = {0};
    ddrss_dimm_spd_t* dimm_spd = {0};
    uint16_t data_len = 0;
    int sts = SILIBS_SUCCESS;

    sts = ddrss_get_config(&ddrss_prd_cfg_knobs);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);

    KNG_DIE_ID die_num = idsw_get_die_id();
    for (uint16_t dimm_local_idx = 0; dimm_local_idx < NUM_DIMM_PER_DIE; dimm_local_idx++)
    {
        dimm_global_idx = (die_num * 6) + dimm_local_idx; // 6 dimm per die: die0- 0 to 5, die1- 6 to 11

        dimm_spd = ddrss_get_dimm_spd(dimm_local_idx);

        /* Length of the structure, 15h for version 2.1, 1Bh for
        version 2.3, 1Ch for version 2.6, 22h for version
        2.7, 28h for version 2.8, 54h for version 3.2, 5Ch
        for version 3.3, 64h for version 3.7 and later and later
        */
        smb_table17.Type = 17;
        smb_table17.Length = 0x5C; // For spec ver 3.7, 64h is correct - But we are conforming to 3.3
        smb_table17.Handle = 0;    // UEFI to populate
        smb_table17.PhysMemoryArrayHandle = 0; // UEFI to populate
        smb_table17.MemoryErrorInfoHandle = 0; // UEFI to populate
        smb_table17.TotalWidth = 80;           // with ECC bits as per PNR
        smb_table17.DataWidth = 64;            // without ECC bits as per PNR
        smb_table17.FormFactor = 0x09;         // DIMM, refer to Table 76 of SMBIOS Ref Spec (DSP0134_3.8.0)
        smb_table17.Size = dimm_capacity_mb;
        smb_table17.ExtendedSize = 0;

        /* Size of the memory device
            If the value is 0, no memory device is installed in
            the socket; if the size is unknown, the field value is
            FFFFh. If the size is 32 GB-1 MB or greater, the
            field value is 7FFFh and the actual size is stored in
            the Extended Size field.
            The granularity in which the value is specified
            depends on the setting of the most-significant bit
            (bit 15). If the bit is 0, the value is specified in
            megabyte units; if the bit is 1, the value is specified
            in kilobyte units. For example, the value 8100h
            identifies a 256 KB memory device and 0100h
            identifies a 256 MB memory device
        */
        if (dimm_capacity_mb >= EXTENDED_DIMM_CAP_LIMIT_MB_TYPE17)
        {
            smb_table17.Size = EXTENDED_DIMM_CAP_LIMIT_MB_TYPE17;
            smb_table17.ExtendedSize = (uint64_t)dimm_capacity_mb;
        }

        // check dimm present
        if (dimm_is_present(dimm_local_idx))
        {
            // Read SPD data from I3C
            if (ddr_i3c_interface_read_spd_nvm_data(&s_i3c_cmd, dimm_local_idx, spd_buff, &data_len, &data_offset, SIZE_64_BYTES) !=
                SILIBS_SUCCESS)
            {
                printf("Error reading DIE=%d DIMM=%d SPD\n", die_num, dimm_local_idx);
            }
        }

        /* Identifies when the Memory Device is one of a set
            of Memory Devices that must be populated with all
            devices of the same type and size, and the set to
            which this device belongs
            A value of 0 indicates that the device is not part of
            a set; a value of FFh indicates that the attribute is
            unknown.
        */
        smb_table17.DeviceSet = 0xFF;
        smb_table17.DeviceLocator = 1; // 1-based index of a list of strings immediately following the type17 table. Ex: SLOT A
        smb_table17.BankLocator = 2; // 1-based index of a list of strings immediately following the type17 table. Ex: BANK 0
        smb_table17.MemoryType = 0x22; // SDRAM, refer to Table 77 of SMBIOS Ref Spec (DSP0134_3.8.0)
        smb_table17.TypeDetail = 0X2000; // Bit 13 is set, Registered (Buffered), refer to Table 78 of SMBIOS Ref Spec (DSP0134)
        smb_table17.Speed = ddrss_get_dimm_speed_mts();
        smb_table17.Manufacturer = 3; // 1-based index of a list of strings immediately following the type17 table
        smb_table17.SerialNumber = 4; // 1-based index of a list of strings immediately following the type17 table
        smb_table17.AssetTag = 5;     // This field will not contain a string
        smb_table17.PartNumber = 6; // 1-based index of a list of strings immediately following the type17 table
        smb_table17.Attributes = ddrss_prd_cfg_knobs.dimm_rank + 1; // 1 for single rank. 2 for dual rank
        smb_table17.ConfiguredMemorySpeed = ddrss_get_dimm_speed_mts();
        smb_table17.MinVoltage =
            ddrss_get_vdd_voltage(); // Defined in JEDEC spec: VDDQ VDD VSS VPP ZQ, (ZQ1) Supply Supply Supply Supply Reference DQ Power Supply: 1.1 V Power Supply: 1.1 V Ground DRAM Activating Power Supply: 1.8 V
        smb_table17.MaxVoltage = ddrss_get_vpp_voltage(); // Defined in JEDEC spec
        smb_table17.ConfiguredVoltage = ddrss_get_vdd_voltage();
        smb_table17.MemoryTechnology = 0x03; // DRAM, refer to Table 79 of SMBIOS Ref Spec (DSP0134_3.8.0)
        smb_table17.MemoryOperatingModeCapability = 8; // Volatile Memory, refer to Table 80 of SMBIOS Ref Spec (DSP0134_3.8.0)
        smb_table17.FirmwareVersion = 7; // 1-based index of a list of strings immediately following the type17 table
        smb_table17.ModuleManufacturerID =
            dimm_spd->sn[0] << 8 | dimm_spd->sn[1]; // The first two bytes of SN is the manufacturer id.
        smb_table17.ModuleProductID = dimm_spd->sn[0] << 8 | dimm_spd->sn[1];
        smb_table17.MemorySubsystemControllerManufacturerID = JEDEC_MICROSOFT_ID; // JEDEC Standard JEP106AV manufacturer IDs
        smb_table17.MemorySubsystemControllerProductID = JEDEC_MICROSOFT_ID;
        smb_table17.NonvolatileSize = 0;
        smb_table17.VolatileSize = (uint64_t)dimm_capacity_kb * 1024; // In Bytes
        smb_table17.CacheSize = 0;
        smb_table17.LogicalSize = 0;
        smb_table17.ExtendedSpeed = 0;
        smb_table17.ExtendedConfiguredMemorySpeed = 0;

        /*-----------------------------------------------------------
                    Copy this DIMM's TYPE17 table to memory
            -----------------------------------------------------------*/
        uint8_t temp_buffer[sizeof(smb_table17)];
        memcpy(temp_buffer, &smb_table17, sizeof(smb_table17));
        for (size_t byte_idx = 0; byte_idx < sizeof(smb_table17); byte_idx++)
        {
            MMIO_WRITE8(this_dies_smbios_next_addr++, temp_buffer[byte_idx]);
        }

        /*-----------------------------------------------------------
                Create and copy string table + extra null string
            -----------------------------------------------------------*/

        // (1) DeviceLocator as silkscreened
        get_deviceLocator_string(dimm_global_idx, tempString, sizeof(tempString));
        for (size_t byte_idx = 0; byte_idx < strlen(tempString) + 1; byte_idx++)
        {
            MMIO_WRITE8(this_dies_smbios_next_addr++, *(tempString + byte_idx));
        }
        memset(tempString, 0, sizeof(tempString));

        // (2) BankLocator
        get_dimm_bank_locator_string(dimm_global_idx, tempString, sizeof(tempString));
        for (size_t byte_idx = 0; byte_idx < strlen(tempString) + 1; byte_idx++)
        {
            MMIO_WRITE8(this_dies_smbios_next_addr++, *(tempString + byte_idx));
        }
        memset(tempString, 0, sizeof(tempString));

        /*-----------------------------------------------------------
            Read subset of the 1KB DIMM SPD data for strings 3 - 5
            -----------------------------------------------------------*/
        uint8_t spd_buff[SIZE_64_BYTES] = {0};
        uint16_t data_offset = START_OF_VENDOR_INFO;
        i3c_cmd_t s_i3c_cmd = {0};
        data_len = 0;

        if (ddr_i3c_interface_read_spd_nvm_data(&s_i3c_cmd, dimm_global_idx, spd_buff, &data_len, &data_offset, SIZE_64_BYTES) !=
            SILIBS_SUCCESS)
        {
            // Continue if error here - data will be all 0s
            printf("Error reading DIMM %d SPD", dimm_global_idx);
        }

        // (3) DIMM (Module) Manufacturer: Variable # of Bytes
        //     Use a look-up from JEDEC JEP-106 for manufacturer ID byte assignment
        //     Only Samsung, Micron, and SK Hynix are translated here
        get_dimm_manufacturer_string(spd_buff, tempString, sizeof(tempString));
        for (size_t byte_idx = 0; byte_idx < strlen(tempString) + 1; byte_idx++)
        {
            MMIO_WRITE8(this_dies_smbios_next_addr++, *(tempString + byte_idx));
        }
        memset(tempString, 0, sizeof(tempString));

        // (4) DIMM (Module) Serial Number: 8 Bytes (will become 16 Bytes + 1 NULL Byte as a string)
        get_dimm_serial_number_string(spd_buff, tempString, sizeof(tempString));
        for (size_t byte_idx = 0; byte_idx < strlen(tempString) + 1; byte_idx++)
        {
            MMIO_WRITE8(this_dies_smbios_next_addr++, *(tempString + byte_idx));
        }
        memset(tempString, 0, sizeof(tempString));

        // (5) AssetTag
        get_dimm_asset_tag_string(tempString, sizeof(tempString));
        for (size_t byte_idx = 0; byte_idx < strlen(tempString) + 1; byte_idx++)
        {
            MMIO_WRITE8(this_dies_smbios_next_addr++, *(tempString + byte_idx));
        }
        memset(tempString, 0, sizeof(tempString));

        // (6) DIMM (Module) Part Number: 30 Bytes - This one is already in ASCII.
        get_dimm_part_number_string(spd_buff, tempString, sizeof(tempString));
        for (size_t byte_idx = 0; byte_idx < strlen(tempString) + 1; byte_idx++)
        {
            MMIO_WRITE8(this_dies_smbios_next_addr++, *(tempString + byte_idx));
        }
        memset(tempString, 0, sizeof(tempString));

        // (7) PHY FW Version
        get_dimm_phy_fw_version_string(tempString, sizeof(tempString));
        for (size_t byte_idx = 0; byte_idx < strlen(tempString) + 1; byte_idx++)
        {
            MMIO_WRITE8(this_dies_smbios_next_addr++, *(tempString + byte_idx));
        }
        memset(tempString, 0, sizeof(tempString));

        // Additional NULL byte to terminate list of strings between TYPE17 tables
        MMIO_WRITE8(this_dies_smbios_next_addr++, 0);

        // Align to next 32bit addr
        if (!FPFW_IS_ALIGNED(this_dies_smbios_next_addr, sizeof(uint32_t)))
        {
            this_dies_smbios_next_addr = FPFW_ALIGN_SIZE_TO_4_BYTES(this_dies_smbios_next_addr);
        }
    }

    printf("SMBIOS 17 DONE\n");
    return this_dies_smbios_next_addr;
}

void ddr_create_smbios_tables(void)
{
    // Get each DIMM's SPD data  from ddrss silicon libs - or read via I3C
    // Get relevant config data (number of DIMMs, rank, speed, etc.)
    // Create SMBIOS Type 16/17
    // Copy to reserved DDR region specified by shared header file
    atu_map_entry_t smbios_mem_atu_map_struct;

    atu_entry_attr_t smbios_test_atu_root_attr = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_NS};

    smbios_mem_atu_map_struct.ap_base_address = SMBIOS_HANDOFF_RESERVATION_BASE;
    smbios_mem_atu_map_struct.mscp_start_address = 0;
    smbios_mem_atu_map_struct.mscp_end_address = SMBIOS_HANDOFF_RESERVATION_SIZE - 1;
    smbios_mem_atu_map_struct.attribute.as_uint32 = smbios_test_atu_root_attr.as_uint32;

    int sts = SILIBS_SUCCESS;
    sts = atu_map(ATU_ID_MSCP, &smbios_mem_atu_map_struct);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);

    uint32_t start_addr = smbios_mem_atu_map_struct.mscp_start_address;
    uint32_t next_addr = start_addr;
    uint32_t size_off = SMBIOS_HANDOFF_RESERVATION_SIZE - sizeof(uint32_t);
    KNG_DIE_ID die_num = idsw_get_die_id();
    if (die_num == DIE_0)
    {
        // Clear memory first
        for (uint32_t i = 0; i < (SMBIOS_HANDOFF_RESERVATION_SIZE / 4); i++)
        {
            MMIO_WRITE32((next_addr + (4 * i)), 0);
        }

        next_addr = ddr_create_smbios_type_16(start_addr);

        next_addr = ddr_create_smbios_type_17(next_addr);
        BUG_ASSERT(next_addr - start_addr < SMBIOS_HANDOFF_RESERVATION_SIZE);

        // Update the SMBIOS length
        MMIO_WRITE32(start_addr + size_off, next_addr - start_addr);

        ddr_wait_for_smbios_sync_point();
    }
    else
    {
        // Wait for D0 to finish building SMBIOS type 16/17.
        // Only after it, we know where to start building SMBIOS table for D1.
        ddr_wait_for_smbios_sync_point();

        // Retrieve the SMBIOS table length built by D0
        uint32_t d0_smbios_len = MMIO_READ32(start_addr + size_off);
        BUG_ASSERT(d0_smbios_len < SMBIOS_HANDOFF_RESERVATION_SIZE);

        // Append SMBIOS type 17 for D1
        next_addr = ddr_create_smbios_type_17(start_addr + d0_smbios_len);
        BUG_ASSERT(next_addr - start_addr + 8 < SMBIOS_HANDOFF_RESERVATION_SIZE);

        // Zero out the next 8 bytes to indicate the end of SMBIOS table
        for (uint32_t i = 0; i < 8; i++)
        {
            MMIO_WRITE8(next_addr + i, 0);
        }
    }

    sts = atu_unmap(ATU_ID_MSCP, &smbios_mem_atu_map_struct);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);

    printf("DDR SMBIOS DONE\n");
}

/**
 Returns the two's complement checksum of all elements in a buffer of
 16-bit values.

 This function first calculates the sum of the 16-bit values in the buffer
 specified by Buffer and Length. The carry bits in the result of addition
 are dropped. Then, the two's complement of the sum is returned. If Length
 is 0, then 0 is returned.

 If Buffer is NULL, then ASSERT().
 If Buffer is not aligned on a 16-bit boundary, then ASSERT().
 If Length is not aligned on a 16-bit boundary, then ASSERT().
 If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

 @param Buffer   The pointer to the buffer to carry out the checksum operation.
 @param Length   The size, in bytes, of Buffer.

 @return Checksum CRC-16 of Buffer.

**/
uint16_t CalculateRemoteCheckSum16(const uint32_t BufferAddr, uint32_t Length)
{
    uint16_t crc = 0xFFFF;              // Initial value
    const uint16_t CRC16_POLY = 0x1021; // Polynomial for CRC-16-CCITT
    uint16_t data;

    if (Length == 0)
    {
        return 0; // Prevent processing empty buffers
    }

    for (uint32_t offset = 0; offset < Length; offset += 2)
    {
        if (((BufferAddr + offset) % 2) == 0)
        {
            // Aligned memory access (safe for direct 16-bit read)
            data = MMIO_READ16(BufferAddr + offset);
        }
        else
        {
            // Unaligned memory access (safe byte-wise reconstruction)
            uint8_t high = MMIO_READ8(BufferAddr + offset);
            uint8_t low = (offset + 1 < Length) ? MMIO_READ8(BufferAddr + offset + 1) : 0;
            data = (uint16_t)(high << 8) | low;
        }

        crc ^= data; // XOR data into CRC

        // Process 16-bit data
        for (uint8_t bit = 0; bit < 16; bit++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ CRC16_POLY;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

const char* dimm_vendor_from_id(uint8_t id_byte)
{
    switch (id_byte)
    {
    case DIMM_SK_HYNIX:
        return dimm_vendor_str[SK_HYNIX_IDX];
    case DIMM_MICRON:
        return dimm_vendor_str[MICRON_IDX];
    case DIMM_SAMSUNG:
        return dimm_vendor_str[SAMSUNG_IDX];
    default:
        return dimm_vendor_str[UNKNOWN_IDX];
    }
}

size_t get_deviceLocator_string(uint32_t dimm_global_idx, char* buffer, size_t bufferSize)
{
    return snprintf(buffer, bufferSize, "SLOT %c", DIMM_SLOT_ID[dimm_global_idx]);
}

size_t get_dimm_bank_locator_string(uint32_t dimm_global_idx, char* buffer, size_t bufferSize)
{
    return snprintf(buffer, bufferSize, "BANK %u", (unsigned)(dimm_global_idx % 2));
}

size_t get_dimm_manufacturer_string(const uint8_t* spd_buff, char* buffer, size_t bufferSize)
{
    uint8_t manufacturer_id = spd_buff[MODULE_MANUFACTURER_SPD_OFFSET - START_OF_VENDOR_INFO];
    const char* vendor = dimm_vendor_from_id(manufacturer_id);
    return snprintf(buffer, bufferSize, "%s", vendor);
}

size_t get_dimm_serial_number_string(const uint8_t* spd_buff, char* buffer, size_t bufferSize)
{
    // Write up to 16 hex digits + null
    return snprintf(buffer,
                    bufferSize,
                    "%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                    spd_buff[SERIALNUM_SPD_OFFSET - START_OF_VENDOR_INFO + 0],
                    spd_buff[SERIALNUM_SPD_OFFSET - START_OF_VENDOR_INFO + 1],
                    spd_buff[SERIALNUM_SPD_OFFSET - START_OF_VENDOR_INFO + 2],
                    spd_buff[SERIALNUM_SPD_OFFSET - START_OF_VENDOR_INFO + 3],
                    spd_buff[SERIALNUM_SPD_OFFSET - START_OF_VENDOR_INFO + 4],
                    spd_buff[SERIALNUM_SPD_OFFSET - START_OF_VENDOR_INFO + 5],
                    spd_buff[SERIALNUM_SPD_OFFSET - START_OF_VENDOR_INFO + 6],
                    spd_buff[SERIALNUM_SPD_OFFSET - START_OF_VENDOR_INFO + 7],
                    spd_buff[SERIALNUM_SPD_OFFSET - START_OF_VENDOR_INFO + 8]);
}

size_t get_dimm_asset_tag_string(char* buffer, size_t bufferSize)
{
    return snprintf(buffer, bufferSize, "N/A");
}

size_t get_dimm_part_number_string(const uint8_t* spd_buff, char* buffer, size_t bufferSize)
{
    if (bufferSize < PARTNUM_NUMBYTES + 1)
    {
        return 0; // Not enough room
    }
    memcpy(buffer, &spd_buff[PARTNUM_SPD_OFFSET - START_OF_VENDOR_INFO], PARTNUM_NUMBYTES);
    buffer[PARTNUM_NUMBYTES] = '\0';
    trim_trailing_whitespace(buffer, PARTNUM_NUMBYTES);

    if (strlen(buffer) == 0)
    {
        return snprintf(buffer, bufferSize, "N/A");
    }

    return strlen(buffer);
}

size_t get_dimm_phy_fw_version_string(char* buffer, size_t bufferSize)
{
    return snprintf(buffer, bufferSize, "0x%08X", (unsigned int)ddrss_get_fw_version());
}

bool dimm_is_present(uint32_t dimm_local_idx)
{
    static uint32_t ddrss_mask = 0;

    if (ddrss_mask == 0)
    {
        // Todo: This is hardcoded to return 0xFFF right now.
        ddrss_mask = get_i3c_dimm_detected();
    }

    return ddrss_mask & (1 << dimm_local_idx) ? true : false;
}
