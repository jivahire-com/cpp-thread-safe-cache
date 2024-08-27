//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * sensor_fifo_mock.h
 * Control settings for mock checks
 */

#pragma once

/*----------- Nested includes ------------*/
#include "device_fifo_id.h"                // for DEVICE_FIFO_MAX_ID
#include "hw_fifo.h"
#include "sensor_fifo_driver_interface.h"   // for sensor_fifo_driver_inf_r...

#include <stdbool.h>
#include <stdint.h>                        // for uint32_t, uint8_t

/*-- Symbolic Constant Macros (defines) --*/
#define FIFO_TIMESTAMP_SIZE (8)

#define PSTATE_FIFO_NUM_ENTRIES (4)
#define PSTATE_FIFO_ENTRY_SIZE_BYTES (16)
#define PSTATE_FIFO_STRIDE_SIZE_BYTES (PSTATE_FIFO_ENTRY_SIZE_BYTES)
#define PSTATE_FIFO_CTRL_EN_MASK (0x1)

#define SCP_MSG_FIFO_NUM_ENTRIES (6)
#define SCP_MSG_FIFO_ENTRY_SIZE_BYTES (32)
#define SCP_MSG_FIFO_STRIDE_SIZE_BYTES (SCP_MSG_FIFO_ENTRY_SIZE_BYTES)
#define SCP_MSG_FIFO_CTRL_EN_MASK (0x2)

#define TILE_TEMP_FIFO_NUM_ENTRIES (8)
#define TILE_TEMP_FIFO_ENTRY_SIZE_BYTES (24)
#define TILE_TEMP_FIFO_STRIDE_SIZE_BYTES (TILE_TEMP_FIFO_ENTRY_SIZE_BYTES * 4)
#define TILE_TEMP_FIFO_CTRL_EN_MASK (0x4)

#define TILE_VOLT_FIFO_NUM_ENTRIES (12)
#define TILE_VOLT_FIFO_ENTRY_SIZE_BYTES (24)
#define TILE_VOLT_FIFO_STRIDE_SIZE_BYTES (TILE_VOLT_FIFO_ENTRY_SIZE_BYTES * 2)
#define TILE_VOLT_FIFO_CTRL_EN_MASK (0x8)

#define CORE_CURRENT_FIFO_NUM_ENTRIES (6)
#define CORE_CURRENT_FIFO_ENTRY_SIZE_BYTES (32)
#define CORE_CURRENT_FIFO_STRIDE_SIZE_BYTES (CORE_CURRENT_FIFO_ENTRY_SIZE_BYTES * 4)
#define CORE_CURRENT_FIFO_CTRL_EN_MASK (0x10)

#define PVT_TEMP_FIFO_NUM_ENTRIES (4)
#define PVT_TEMP_FIFO_ENTRY_SIZE_BYTES (32)
#define PVT_TEMP_FIFO_STRIDE_SIZE_BYTES (PVT_TEMP_FIFO_ENTRY_SIZE_BYTES)
#define PVT_TEMP_FIFO_CTRL_EN_MASK (0x11)

#define PVT_VOLT_FIFO_NUM_ENTRIES (12)
#define PVT_VOLT_FIFO_ENTRY_SIZE_BYTES (32)
#define PVT_VOLT_FIFO_STRIDE_SIZE_BYTES (PVT_VOLT_FIFO_ENTRY_SIZE_BYTES)
#define PVT_VOLT_FIFO_CTRL_EN_MASK (0x12)

#define DIMM_FIFO_NUM_ENTRIES (14)
#define DIMM_FIFO_ENTRY_SIZE_BYTES (32)
#define DIMM_FIFO_STRIDE_SIZE_BYTES (DIMM_FIFO_ENTRY_SIZE_BYTES)
#define DIMM_FIFO_CTRL_EN_MASK (0x14)

#define VR_TEMP_FIFO_NUM_ENTRIES (8)
#define VR_TEMP_FIFO_ENTRY_SIZE_BYTES (32)
#define VR_TEMP_FIFO_STRIDE_SIZE_BYTES (VR_TEMP_FIFO_ENTRY_SIZE_BYTES)
#define VR_TEMP_FIFO_CTRL_EN_MASK (0x18)

#define VR_CURRENT_FIFO_NUM_ENTRIES (6)
#define VR_CURRENT_FIFO_ENTRY_SIZE_BYTES (32)
#define VR_CURRENT_FIFO_STRIDE_SIZE_BYTES (VR_CURRENT_FIFO_ENTRY_SIZE_BYTES)
#define VR_CURRENT_FIFO_CTRL_EN_MASK (0x20)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
extern bool snsr_fifo_mock_check_mmio_inputs;
extern bool snsr_fifo_mock_use_real_mmio;


typedef struct
{
    uint8_t __attribute__((aligned(8))) pstate_fifo[PSTATE_FIFO_NUM_ENTRIES * PSTATE_FIFO_STRIDE_SIZE_BYTES];
    uint8_t __attribute__((aligned(8))) msg_fifo[SCP_MSG_FIFO_NUM_ENTRIES * SCP_MSG_FIFO_STRIDE_SIZE_BYTES];
    uint8_t __attribute__((aligned(8))) tile_temp_fifo[TILE_TEMP_FIFO_NUM_ENTRIES * TILE_TEMP_FIFO_STRIDE_SIZE_BYTES];
    uint8_t __attribute__((aligned(8))) tile_volt_fifo[TILE_VOLT_FIFO_NUM_ENTRIES * TILE_VOLT_FIFO_STRIDE_SIZE_BYTES];
    uint8_t __attribute__((aligned(8))) core_current_fifo[CORE_CURRENT_FIFO_NUM_ENTRIES * CORE_CURRENT_FIFO_STRIDE_SIZE_BYTES];
    uint8_t __attribute__((aligned(8))) pvt_temp_fifo[PVT_TEMP_FIFO_NUM_ENTRIES * PVT_TEMP_FIFO_STRIDE_SIZE_BYTES];
    uint8_t __attribute__((aligned(8))) pvt_volt_fifo[PVT_VOLT_FIFO_NUM_ENTRIES * PVT_VOLT_FIFO_STRIDE_SIZE_BYTES];
    uint8_t __attribute__((aligned(8))) dimm_temp_fifo[DIMM_FIFO_NUM_ENTRIES * DIMM_FIFO_STRIDE_SIZE_BYTES];
    uint8_t __attribute__((aligned(8))) vr_temp_fifo[VR_TEMP_FIFO_NUM_ENTRIES * VR_TEMP_FIFO_STRIDE_SIZE_BYTES];
    uint8_t __attribute__((aligned(8))) vr_curr_fifo[VR_CURRENT_FIFO_NUM_ENTRIES * VR_CURRENT_FIFO_STRIDE_SIZE_BYTES];
} sensor_fifo_mem_t;
extern sensor_fifo_mem_t fifo_mem;

extern uint32_t pstate_fifo_ctrl_reg;
extern uint32_t msg_fifo_ctrl_reg;
extern uint32_t tile_temp_fifo_ctrl_reg;
extern uint32_t tile_volt_fifo_ctrl_reg;
extern uint32_t core_current_fifo_ctrl_reg;

extern uint32_t pstate_fifo_read_reg;
extern uint32_t msg_fifo_read_reg;
extern uint32_t tile_temp_fifo_read_reg;
extern uint32_t tile_volt_fifo_read_reg;
extern uint32_t core_current_fifo_read_reg;
extern uint32_t pvt_temp_fifo_read_reg;
extern uint32_t pvt_volt_fifo_read_reg;
extern uint32_t dimm_temp_fifo_read_reg;
extern uint32_t vr_temp_fifo_read_reg;
extern uint32_t vr_curr_fifo_read_reg;

extern uint32_t pstate_fifo_write_reg;
extern uint32_t msg_fifo_write_reg;
extern uint32_t tile_temp_fifo_write_reg;
extern uint32_t tile_volt_fifo_write_reg;
extern uint32_t core_current_fifo_write_reg;
extern uint32_t pvt_temp_fifo_write_reg;
extern uint32_t pvt_volt_fifo_write_reg;
extern uint32_t dimm_temp_fifo_write_reg;
extern uint32_t vr_temp_fifo_write_reg;
extern uint32_t vr_curr_fifo_write_reg;


extern sensor_fifo_device_properties_t test_fifo_properties[DEVICE_FIFO_MAX_ID];
extern sensor_fifo_control_t test_hw_fifo_control[DEVICE_FIFO_MAX_ID];
/*--------- Function Prototypes ----------*/
void initialize_mock_fifos(void);