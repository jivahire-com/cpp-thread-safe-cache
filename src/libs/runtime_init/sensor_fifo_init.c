//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_init.c
 * Instantiates Sensor Fifo driver and Service
 */

/*----------- Nested includes ------------*/


#include <DfwkThreadXHost.h>     // for PDFWK_THREADX_HOST
#include <device_fifo_id.h>
#include <fpfw_init.h>           // for FPFW_INIT_STATUS_SUCCESS, fpfw_init_get...
#include <idsw.h>                // for idsw_get_die_id
#include <idsw_kng.h>
#include <scf_mhu_device.h>
#include <sensor_fifo_cli_service.h>
#include <sensor_fifo_driver_interface.h>
#include <sensor_fifo_service.h>
#include <sensor_fifo_service_init.h>
#include <silibs_mcp_exp_top_regs.h> // IWYU pragma: keep
#include <silibs_mcp_top_regs.h>     // IWYU pragma: keep
#include <silibs_scp_exp_top_regs.h> // IWYU pragma: keep
#include <silibs_scp_top_regs.h>     // IWYU pragma: keep
#include <stddef.h>              // for NULL
#include <telemetry_defines.h>

/*-- Symbolic Constant Macros (defines) --*/

#if defined (SCP_RUNTIME_INIT)
  #define SCF_MHU_BASE_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_MHU_ADDRESS)
  #define SCF_RAM_BASE_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS)
  #define SCF_EXP_CSR_BASE_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS)
  #define IS_SCP (true)

#elif defined (MCP_RUNTIME_INIT)
  #define SCF_MHU_BASE_ADDRESS (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_MHU_ADDRESS)
  #define SCF_RAM_BASE_ADDRESS (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_RAM_ADDRESS)
  #define SCF_EXP_CSR_BASE_ADDRESS (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_MCP_EXP_CSR_ADDRESS)
  #define IS_SCP (false)
#else
  #error runtime not defined
#endif

// NOTE:  for the five hardware fifo's, the SCF hardware adds a timestamp before every entry. The hardware requires
// that the start address be programmed with an 8 byte offset to account for the timestamp.  That is handled
// internally by the firmware. Therefore all of the configurations below assign the actual fifo start address to START_ADDR.
// END_ADDR is the end of the fifo plus 1 byte.  That allows the size to be simply calculated by END_ADDR - START_ADDR.
// The inclusive end address is END_ADDR - 1.  Hardware that requires the inclusive end address is handled internally by the
// firmware.

static_assert((int)SENSOR_FIFO_MAX_ID == (int)DEVICE_FIFO_MAX_ID, "SENSOR_FIFO_MAX_ID != DEVICE_FIFO_MAX_ID");

#define SCF_RAM_BUFFER_SIZE_BYTES (65536)

#define PSTATE_FIFO_NUM_ENTRIES               (1152)
#define PSTATE_FIFO_ENTRY_SIZE_BYTES          (BUFFER_ADDRESS_INC_2QW)
#define PSTATE_FIFO_STRIDE_SIZE_BYTES         (BUFFER_ADDRESS_INC_2QW)
#define PSTATE_FIFO_START_ADDR                (SCF_RAM_BASE_ADDRESS)
#define PSTATE_FIFO_END_ADDR                  (PSTATE_FIFO_START_ADDR + (PSTATE_FIFO_NUM_ENTRIES * PSTATE_FIFO_STRIDE_SIZE_BYTES))
static_assert((PSTATE_FIFO_NUM_ENTRIES % 128) == 0, "PSTATE_FIFO_NUM_ENTRIES needs to be multiple of 128");

#define SCP_MSG_FIFO_NUM_ENTRIES              (256)
#define SCP_MSG_FIFO_ENTRY_SIZE_BYTES         (BUFFER_ADDRESS_INC_2QW)
#define SCP_MSG_FIFO_STRIDE_SIZE_BYTES        (BUFFER_ADDRESS_INC_2QW)
#define SCP_MSG_FIFO_START_ADDR               (PSTATE_FIFO_END_ADDR)
#define SCP_MSG_FIFO_END_ADDR                 (SCP_MSG_FIFO_START_ADDR + (SCP_MSG_FIFO_NUM_ENTRIES * SCP_MSG_FIFO_STRIDE_SIZE_BYTES))
static_assert((SCP_MSG_FIFO_NUM_ENTRIES % 128) == 0, "SCP_MSG_FIFO_NUM_ENTRIES needs to be multiple of 128");

#define TILE_TEMP_FIFO_NUM_ENTRIES            (8)   // number of strides
#define TILE_TEMP_FIFO_ENTRY_SIZE_BYTES       (BUFFER_ADDRESS_INC_4QW)
#define TILE_TEMP_FIFO_STRIDE_SIZE_BYTES      (TILE_TEMP_FIFO_ENTRY_SIZE_BYTES * NUM_CPU_TILES )
#define TILE_TEMP_FIFO_START_ADDR             (SCP_MSG_FIFO_END_ADDR)
#define TILE_TEMP_FIFO_END_ADDR               (TILE_TEMP_FIFO_START_ADDR + (TILE_TEMP_FIFO_NUM_ENTRIES * TILE_TEMP_FIFO_STRIDE_SIZE_BYTES))
static_assert((TILE_TEMP_FIFO_NUM_ENTRIES <= 16), "TILE_TEMP_FIFO_NUM_ENTRIES <= 16");

#define TILE_VOLT_FIFO_NUM_ENTRIES            (8)   // number of strides
#define TILE_VOLT_FIFO_ENTRY_SIZE_BYTES       (BUFFER_ADDRESS_INC_2QW)
#define TILE_VOLT_FIFO_STRIDE_SIZE_BYTES      (TILE_VOLT_FIFO_ENTRY_SIZE_BYTES * NUM_CPU_TILES )
#define TILE_VOLT_FIFO_START_ADDR             (TILE_TEMP_FIFO_END_ADDR)
#define TILE_VOLT_FIFO_END_ADDR               (TILE_VOLT_FIFO_START_ADDR + (TILE_VOLT_FIFO_NUM_ENTRIES * TILE_VOLT_FIFO_STRIDE_SIZE_BYTES))
static_assert((TILE_VOLT_FIFO_NUM_ENTRIES <= 16), "TILE_VOLT_FIFO_NUM_ENTRIES <= 16");

#define CORE_CURRENT_FIFO_NUM_ENTRIES         (16)   // number of strides
#define CORE_CURRENT_FIFO_ENTRY_SIZE_BYTES    (BUFFER_ADDRESS_INC_2QW)
#define CORE_CURRENT_FIFO_STRIDE_SIZE_BYTES   (CORE_CURRENT_FIFO_ENTRY_SIZE_BYTES * NUM_AP_CORES_PER_DIE )
#define CORE_CURRENT_FIFO_START_ADDR          (TILE_VOLT_FIFO_END_ADDR)
#define CORE_CURRENT_FIFO_END_ADDR            (CORE_CURRENT_FIFO_START_ADDR + (CORE_CURRENT_FIFO_NUM_ENTRIES * CORE_CURRENT_FIFO_STRIDE_SIZE_BYTES))
static_assert((CORE_CURRENT_FIFO_NUM_ENTRIES <= 16), "CORE_CURRENT_FIFO_NUM_ENTRIES <= 16");

#define PVT_TEMP_FIFO_NUM_ENTRIES             (8)
#define PVT_TEMP_FIFO_ENTRY_SIZE_BYTES        (QUADWORD_ADDRESS_SIZE * 5U)
#define PVT_TEMP_FIFO_STRIDE_SIZE_BYTES       (QUADWORD_ADDRESS_SIZE * 5U)
#define PVT_TEMP_FIFO_START_ADDR              (CORE_CURRENT_FIFO_END_ADDR )
#define PVT_TEMP_FIFO_END_ADDR                (PVT_TEMP_FIFO_START_ADDR + (PVT_TEMP_FIFO_NUM_ENTRIES * PVT_TEMP_FIFO_STRIDE_SIZE_BYTES))

#define PVT_VOLT_FIFO_NUM_ENTRIES             (8)
#define PVT_VOLT_FIFO_ENTRY_SIZE_BYTES        (QUADWORD_ADDRESS_SIZE * 6U)
#define PVT_VOLT_FIFO_STRIDE_SIZE_BYTES       (QUADWORD_ADDRESS_SIZE * 6U)
#define PVT_VOLT_FIFO_START_ADDR              (PVT_TEMP_FIFO_END_ADDR)
#define PVT_VOLT_FIFO_END_ADDR                (PVT_VOLT_FIFO_START_ADDR + (PVT_VOLT_FIFO_NUM_ENTRIES * PVT_VOLT_FIFO_STRIDE_SIZE_BYTES))

#define DIMM_FIFO_NUM_ENTRIES                 (8)
#define DIMM_FIFO_ENTRY_SIZE_BYTES            (BUFFER_ADDRESS_INC_3QW)
#define DIMM_FIFO_STRIDE_SIZE_BYTES           (DIMM_FIFO_ENTRY_SIZE_BYTES * 12U) //12 channels
#define DIMM_FIFO_START_ADDR                  (PVT_VOLT_FIFO_END_ADDR)
#define DIMM_FIFO_END_ADDR                    (DIMM_FIFO_START_ADDR + (DIMM_FIFO_NUM_ENTRIES * DIMM_FIFO_STRIDE_SIZE_BYTES))

#define VR_TEMP_FIFO_NUM_ENTRIES              (24)
#define VR_TEMP_FIFO_ENTRY_SIZE_BYTES         (BUFFER_ADDRESS_INC_3QW)
#define VR_TEMP_FIFO_STRIDE_SIZE_BYTES        (VR_TEMP_FIFO_ENTRY_SIZE_BYTES)
#define VR_TEMP_FIFO_START_ADDR               (DIMM_FIFO_END_ADDR )
#define VR_TEMP_FIFO_END_ADDR                 (VR_TEMP_FIFO_START_ADDR + (VR_TEMP_FIFO_NUM_ENTRIES * VR_TEMP_FIFO_STRIDE_SIZE_BYTES))

#define VR_CURRENT_FIFO_NUM_ENTRIES           (24)
#define VR_CURRENT_FIFO_ENTRY_SIZE_BYTES      (QUADWORD_ADDRESS_SIZE * 5U)
#define VR_CURRENT_FIFO_STRIDE_SIZE_BYTES     (QUADWORD_ADDRESS_SIZE * 5U)
#define VR_CURRENT_FIFO_START_ADDR            (VR_TEMP_FIFO_END_ADDR)
#define VR_CURRENT_FIFO_END_ADDR              (VR_CURRENT_FIFO_START_ADDR + (VR_CURRENT_FIFO_NUM_ENTRIES * VR_CURRENT_FIFO_STRIDE_SIZE_BYTES))

static_assert(VR_CURRENT_FIFO_END_ADDR < (SCF_RAM_BASE_ADDRESS + SCF_RAM_BUFFER_SIZE_BYTES), "Exceeded SCF RAM capacity");


#define FPGA_SCF_RAM_BUFFER_SIZE_BYTES (32768)

#define FPGA_PSTATE_FIFO_NUM_ENTRIES               (512)
#define FPGA_PSTATE_FIFO_START_ADDR                (SCF_RAM_BASE_ADDRESS)
#define FPGA_PSTATE_FIFO_END_ADDR                  (FPGA_PSTATE_FIFO_START_ADDR + (FPGA_PSTATE_FIFO_NUM_ENTRIES * PSTATE_FIFO_STRIDE_SIZE_BYTES))
static_assert((FPGA_PSTATE_FIFO_NUM_ENTRIES % 128) == 0, "FPGA_PSTATE_FIFO_NUM_ENTRIES needs to be multiple of 128");

#define FPGA_SCP_MSG_FIFO_NUM_ENTRIES              (128)
#define FPGA_SCP_MSG_FIFO_START_ADDR               (FPGA_PSTATE_FIFO_END_ADDR)
#define FPGA_SCP_MSG_FIFO_END_ADDR                 (FPGA_SCP_MSG_FIFO_START_ADDR + (FPGA_SCP_MSG_FIFO_NUM_ENTRIES * SCP_MSG_FIFO_STRIDE_SIZE_BYTES))
static_assert((FPGA_SCP_MSG_FIFO_NUM_ENTRIES % 128) == 0, "FPGA_SCP_MSG_FIFO_NUM_ENTRIES needs to be multiple of 128");

#define FPGA_TILE_TEMP_FIFO_NUM_ENTRIES            (4)   // number of strides
#define FPGA_TILE_TEMP_FIFO_START_ADDR             (FPGA_SCP_MSG_FIFO_END_ADDR)
#define FPGA_TILE_TEMP_FIFO_END_ADDR               (FPGA_TILE_TEMP_FIFO_START_ADDR + (FPGA_TILE_TEMP_FIFO_NUM_ENTRIES * TILE_TEMP_FIFO_STRIDE_SIZE_BYTES))
static_assert((FPGA_TILE_TEMP_FIFO_NUM_ENTRIES <= 16), "FPGA_TILE_TEMP_FIFO_NUM_ENTRIES <= 16");

#define FPGA_TILE_VOLT_FIFO_NUM_ENTRIES            (4)   // number of strides
#define FPGA_TILE_VOLT_FIFO_START_ADDR             (FPGA_TILE_TEMP_FIFO_END_ADDR)
#define FPGA_TILE_VOLT_FIFO_END_ADDR               (FPGA_TILE_VOLT_FIFO_START_ADDR + (FPGA_TILE_VOLT_FIFO_NUM_ENTRIES * TILE_VOLT_FIFO_STRIDE_SIZE_BYTES))
static_assert((FPGA_TILE_VOLT_FIFO_NUM_ENTRIES <= 16), "FPGA_TILE_VOLT_FIFO_NUM_ENTRIES <= 16");

#define FPGA_CORE_CURRENT_FIFO_NUM_ENTRIES         (8)   // number of strides
#define FPGA_CORE_CURRENT_FIFO_START_ADDR          (FPGA_TILE_VOLT_FIFO_END_ADDR)
#define FPGA_CORE_CURRENT_FIFO_END_ADDR            (FPGA_CORE_CURRENT_FIFO_START_ADDR + (FPGA_CORE_CURRENT_FIFO_NUM_ENTRIES * CORE_CURRENT_FIFO_STRIDE_SIZE_BYTES))
static_assert((FPGA_CORE_CURRENT_FIFO_NUM_ENTRIES <= 16), "FPGA_CORE_CURRENT_FIFO_NUM_ENTRIES <= 16");

// FPGA number of entries is the same for the following, but the memory map still needs a shift
#define FPGA_PVT_TEMP_FIFO_START_ADDR              (FPGA_CORE_CURRENT_FIFO_END_ADDR)
#define FPGA_PVT_TEMP_FIFO_END_ADDR                (FPGA_PVT_TEMP_FIFO_START_ADDR + (PVT_TEMP_FIFO_NUM_ENTRIES * PVT_TEMP_FIFO_STRIDE_SIZE_BYTES))

#define FPGA_PVT_VOLT_FIFO_START_ADDR              (FPGA_PVT_TEMP_FIFO_END_ADDR)
#define FPGA_PVT_VOLT_FIFO_END_ADDR                (FPGA_PVT_VOLT_FIFO_START_ADDR + (PVT_VOLT_FIFO_NUM_ENTRIES * PVT_VOLT_FIFO_STRIDE_SIZE_BYTES))

#define FPGA_DIMM_FIFO_START_ADDR                  (FPGA_PVT_VOLT_FIFO_END_ADDR)
#define FPGA_DIMM_FIFO_END_ADDR                    (FPGA_DIMM_FIFO_START_ADDR + (DIMM_FIFO_NUM_ENTRIES * DIMM_FIFO_STRIDE_SIZE_BYTES))

#define FPGA_VR_TEMP_FIFO_START_ADDR               (FPGA_DIMM_FIFO_END_ADDR)
#define FPGA_VR_TEMP_FIFO_END_ADDR                 (FPGA_VR_TEMP_FIFO_START_ADDR + (VR_TEMP_FIFO_NUM_ENTRIES * VR_TEMP_FIFO_STRIDE_SIZE_BYTES))

#define FPGA_VR_CURRENT_FIFO_START_ADDR            (FPGA_VR_TEMP_FIFO_END_ADDR)
#define FPGA_VR_CURRENT_FIFO_END_ADDR              (FPGA_VR_CURRENT_FIFO_START_ADDR + (VR_CURRENT_FIFO_NUM_ENTRIES * VR_CURRENT_FIFO_STRIDE_SIZE_BYTES))

static_assert(FPGA_VR_CURRENT_FIFO_END_ADDR < (SCF_RAM_BASE_ADDRESS + FPGA_SCF_RAM_BUFFER_SIZE_BYTES), "Exceeded FPGA SCF RAM capacity");

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static scf_mhu_device_config_t s_scf_mhu_device_cfg =
{
    .scf_mhu_base_address     = SCF_MHU_BASE_ADDRESS,
    .scf_exp_csr_base_address = SCF_EXP_CSR_BASE_ADDRESS,
    .scf_ram_base_address     = SCF_RAM_BASE_ADDRESS,
    .scf_ram_buffer_size      = SCF_RAM_BUFFER_SIZE_BYTES,
    .is_scp                   = IS_SCP
};

/**
 * @brief Needs to be present life of device as the entires are actively used during operation
 *
 */
static sensor_fifo_device_properties_t s_fifo_properties[SENSOR_FIFO_MAX_ID] = {
    [SENSOR_FIFO_PSTATE_TELEMETRY_HW] = {
                                                .device_fifo_id = DEVICE_FIFO_PSTATE_TLM_HW_PROD,
                                                .entry_count = PSTATE_FIFO_NUM_ENTRIES,
                                                .entry_size_bytes = PSTATE_FIFO_ENTRY_SIZE_BYTES,
                                                .stride_size_bytes = PSTATE_FIFO_STRIDE_SIZE_BYTES,
                                                .start_address_incl = PSTATE_FIFO_START_ADDR,
                                                .end_address_excl = PSTATE_FIFO_END_ADDR,
                                                .name = "PSTATE Fifo",
                                                },
    [SENSOR_FIFO_SCP_MSG_TELEMETRY_HW] = {
                                                .device_fifo_id = DEVICE_FIFO_SCP_MSG_TLM_HW_PROD,
                                                .entry_count = SCP_MSG_FIFO_NUM_ENTRIES,
                                                .entry_size_bytes = SCP_MSG_FIFO_ENTRY_SIZE_BYTES,
                                                .stride_size_bytes = SCP_MSG_FIFO_STRIDE_SIZE_BYTES,
                                                .start_address_incl = SCP_MSG_FIFO_START_ADDR,
                                                .end_address_excl = SCP_MSG_FIFO_END_ADDR,
                                                .name = "SCP Msg Fifo",
                                                },
    [SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW] = {
                                                .device_fifo_id = DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD,
                                                .entry_count = TILE_TEMP_FIFO_NUM_ENTRIES,
                                                .entry_size_bytes = TILE_TEMP_FIFO_ENTRY_SIZE_BYTES,
                                                .stride_size_bytes = TILE_TEMP_FIFO_STRIDE_SIZE_BYTES,
                                                .start_address_incl = TILE_TEMP_FIFO_START_ADDR,
                                                .end_address_excl = TILE_TEMP_FIFO_END_ADDR,
                                                .name = "Tile Temperature Fifo",
                                                },
    [SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW] = {
                                                .device_fifo_id = DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD,
                                                .entry_count = TILE_VOLT_FIFO_NUM_ENTRIES,
                                                .entry_size_bytes = TILE_VOLT_FIFO_ENTRY_SIZE_BYTES,
                                                .stride_size_bytes = TILE_VOLT_FIFO_STRIDE_SIZE_BYTES,
                                                .start_address_incl = TILE_VOLT_FIFO_START_ADDR,
                                                .end_address_excl = TILE_VOLT_FIFO_END_ADDR,
                                                .name = "Tile Voltage Fifo",
                                                },
    [SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW] = {
                                                .device_fifo_id = DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD,
                                                .entry_count = CORE_CURRENT_FIFO_NUM_ENTRIES,
                                                .entry_size_bytes = CORE_CURRENT_FIFO_ENTRY_SIZE_BYTES,
                                                .stride_size_bytes = CORE_CURRENT_FIFO_STRIDE_SIZE_BYTES,
                                                .start_address_incl = CORE_CURRENT_FIFO_START_ADDR,
                                                .end_address_excl = CORE_CURRENT_FIFO_END_ADDR,
                                                .name = "Core Current Fifo",
                                                },
    [SENSOR_FIFO_PVT_TEMP_FW] = {
                                                .device_fifo_id = DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD,
                                                .entry_count = PVT_TEMP_FIFO_NUM_ENTRIES,
                                                .entry_size_bytes = PVT_TEMP_FIFO_ENTRY_SIZE_BYTES,
                                                .stride_size_bytes = PVT_TEMP_FIFO_STRIDE_SIZE_BYTES,
                                                .start_address_incl = PVT_TEMP_FIFO_START_ADDR,
                                                .end_address_excl = PVT_TEMP_FIFO_END_ADDR,
                                                .name = "PVT Temperature Fifo",
                                                },

    [SENSOR_FIFO_PVT_VOLTAGE_FW] = {
                                                .device_fifo_id = DEVICE_FIFO_PVT_VOLT_TLM_FW_PROD,
                                                .entry_count = PVT_VOLT_FIFO_NUM_ENTRIES,
                                                .entry_size_bytes = PVT_VOLT_FIFO_ENTRY_SIZE_BYTES,
                                                .stride_size_bytes = PVT_VOLT_FIFO_STRIDE_SIZE_BYTES,
                                                .start_address_incl = PVT_VOLT_FIFO_START_ADDR,
                                                .end_address_excl = PVT_VOLT_FIFO_END_ADDR,
                                                .name = "PVT Voltage Fifo",
                                                },
    [SENSOR_FIFO_DIMM_TEMP_FW] = {
                                                .device_fifo_id = DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD,
                                                .entry_count = DIMM_FIFO_NUM_ENTRIES,
                                                .entry_size_bytes = DIMM_FIFO_ENTRY_SIZE_BYTES,
                                                .stride_size_bytes = DIMM_FIFO_STRIDE_SIZE_BYTES,
                                                .start_address_incl = DIMM_FIFO_START_ADDR,
                                                .end_address_excl = DIMM_FIFO_END_ADDR,
                                                .name = "DIMM Fifo",
                                                },
    [SENSOR_FIFO_VR_TEMP_FW] = {
                                                .device_fifo_id = DEVICE_FIFO_VR_TEMP_TLM_FW_PROD,
                                                .entry_count = VR_TEMP_FIFO_NUM_ENTRIES,
                                                .entry_size_bytes = VR_TEMP_FIFO_ENTRY_SIZE_BYTES,
                                                .stride_size_bytes = VR_TEMP_FIFO_STRIDE_SIZE_BYTES,
                                                .start_address_incl = VR_TEMP_FIFO_START_ADDR,
                                                .end_address_excl = VR_TEMP_FIFO_END_ADDR,
                                                .name = "VR Temp Fifo",
                                                },
    [SENSOR_FIFO_VR_CURRENT_FW] = {
                                                .device_fifo_id = DEVICE_FIFO_VR_CURRENT_TLM_FW_PROD,
                                                .entry_count = VR_CURRENT_FIFO_NUM_ENTRIES,
                                                .entry_size_bytes = VR_CURRENT_FIFO_ENTRY_SIZE_BYTES,
                                                .stride_size_bytes = VR_CURRENT_FIFO_STRIDE_SIZE_BYTES,
                                                .start_address_incl = VR_CURRENT_FIFO_START_ADDR,
                                                .end_address_excl = VR_CURRENT_FIFO_END_ADDR,
                                                .name = "VR Current Fifo",
                                                },
};


/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(sensor_fifo, FPFW_INIT_DEPENDENCIES("dfwk","hw_ver","std_io"))
{
    switch (idsw_get_platform_sdv())
    {
        // fall-thru intended
    case PLATFORM_FPGA:
    case PLATFORM_FPGA_TINY:
    case PLATFORM_FPGA_SMALL:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:

        s_fifo_properties[SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW].entry_count = FPGA_TILE_TEMP_FIFO_NUM_ENTRIES;
        s_fifo_properties[SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW].start_address_incl = FPGA_TILE_TEMP_FIFO_START_ADDR;
        s_fifo_properties[SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW].end_address_excl = FPGA_TILE_TEMP_FIFO_END_ADDR;

        s_fifo_properties[SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW].entry_count = FPGA_TILE_VOLT_FIFO_NUM_ENTRIES;
        s_fifo_properties[SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW].start_address_incl = FPGA_TILE_VOLT_FIFO_START_ADDR;
        s_fifo_properties[SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW].end_address_excl = FPGA_TILE_VOLT_FIFO_END_ADDR;

        s_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].entry_count = FPGA_CORE_CURRENT_FIFO_NUM_ENTRIES;
        s_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].start_address_incl = FPGA_CORE_CURRENT_FIFO_START_ADDR;
        s_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].end_address_excl = FPGA_CORE_CURRENT_FIFO_END_ADDR;

        s_fifo_properties[SENSOR_FIFO_PSTATE_TELEMETRY_HW].entry_count = FPGA_PSTATE_FIFO_NUM_ENTRIES;
        s_fifo_properties[SENSOR_FIFO_PSTATE_TELEMETRY_HW].start_address_incl = FPGA_PSTATE_FIFO_START_ADDR;
        s_fifo_properties[SENSOR_FIFO_PSTATE_TELEMETRY_HW].end_address_excl = FPGA_PSTATE_FIFO_END_ADDR;

        s_fifo_properties[SENSOR_FIFO_SCP_MSG_TELEMETRY_HW].entry_count = FPGA_SCP_MSG_FIFO_NUM_ENTRIES;
        s_fifo_properties[SENSOR_FIFO_SCP_MSG_TELEMETRY_HW].start_address_incl = FPGA_SCP_MSG_FIFO_START_ADDR;
        s_fifo_properties[SENSOR_FIFO_SCP_MSG_TELEMETRY_HW].end_address_excl = FPGA_SCP_MSG_FIFO_END_ADDR;

        s_fifo_properties[SENSOR_FIFO_PVT_TEMP_FW].start_address_incl = FPGA_PVT_TEMP_FIFO_START_ADDR;
        s_fifo_properties[SENSOR_FIFO_PVT_TEMP_FW].end_address_excl = FPGA_PVT_TEMP_FIFO_END_ADDR;

        s_fifo_properties[SENSOR_FIFO_PVT_VOLTAGE_FW].start_address_incl = FPGA_PVT_VOLT_FIFO_START_ADDR;
        s_fifo_properties[SENSOR_FIFO_PVT_VOLTAGE_FW].end_address_excl = FPGA_PVT_VOLT_FIFO_END_ADDR;

        s_fifo_properties[SENSOR_FIFO_DIMM_TEMP_FW].start_address_incl = FPGA_DIMM_FIFO_START_ADDR;
        s_fifo_properties[SENSOR_FIFO_DIMM_TEMP_FW].end_address_excl = FPGA_DIMM_FIFO_END_ADDR;

        s_fifo_properties[SENSOR_FIFO_VR_TEMP_FW].start_address_incl = FPGA_VR_TEMP_FIFO_START_ADDR;
        s_fifo_properties[SENSOR_FIFO_VR_TEMP_FW].end_address_excl = FPGA_VR_TEMP_FIFO_END_ADDR;

        s_fifo_properties[SENSOR_FIFO_VR_CURRENT_FW].start_address_incl = FPGA_VR_CURRENT_FIFO_START_ADDR;
        s_fifo_properties[SENSOR_FIFO_VR_CURRENT_FW].end_address_excl = FPGA_VR_CURRENT_FIFO_END_ADDR;

        s_scf_mhu_device_cfg.scf_ram_buffer_size = FPGA_SCF_RAM_BUFFER_SIZE_BYTES;

    default:
        break;
    }

    // get driver fwk threadx handle
    fpfw_init_component_id_t dfwk_id = "dfwk";
    PDFWK_THREADX_HOST drvfwk = (PDFWK_THREADX_HOST)fpfw_init_get_handle(dfwk_id);

    /// TODO:  from fuse service get info for the following
    s_scf_mhu_device_cfg.tile_mask = 0;
    s_scf_mhu_device_cfg.core_mask_lo = 0;
    s_scf_mhu_device_cfg.core_mask_hi = 0;

    static scf_mhu_device_t scf_mhu_device = {0};
    scf_mhu_device_initialize(&scf_mhu_device, &drvfwk->Schedule, s_fifo_properties, ARRAY_SIZE(s_fifo_properties), &s_scf_mhu_device_cfg);

    static sensor_fifo_driver_interface_t sensor_fifo_driver_interface;
    sensor_fifo_driver_inf_init(&sensor_fifo_driver_interface, (sensor_fifo_device_t*)&scf_mhu_device);

    sensor_fifo_svc_initialize(&sensor_fifo_driver_interface);

    sensor_fifo_cli_svc_initialize(&sensor_fifo_driver_interface);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
