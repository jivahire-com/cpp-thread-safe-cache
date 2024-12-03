//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mesh_error_handler.h
 *  Mesh Error Handler Definitions
 */

#pragma once

/*--------------- Includes ---------------*/

/*
"[Event Data1]
[7:6] ‘b10 = OEM data in Event Data 2
[5:4] ‘b10 = OEM data in Event Data 3
[3:0] ECC error data type
0x0 = Mesh Uncorrectable Non-Fatal Error
0x1 = Mesh Uncorrectable Fatal Error
0x2 = Mesh Correctable Error
0x3 = Mesh Uncorrectable Non-Fatal Fault
0x4 = Mesh Uncorrectable Fatal Fault
0x5 = Mesh Correctable Fault
*/
#define MESH_UNCORRECTABLE_NON_FATAL_ERROR ((2 << 6) | (2 << 4) | 0)
#define MESH_UNCORRECTABLE_FATAL_ERROR     ((2 << 6) | (2 << 4) | 1)
#define MESH_CORRECTABLE_ERROR             ((2 << 6) | (2 << 4) | 2)
#define MESH_UNCORRECTABLE_NON_FATAL_FAULT ((2 << 6) | (2 << 4) | 3)
#define MESH_UNCORRECTABLE_FATAL_FAULT     ((2 << 6) | (2 << 4) | 4)
#define MESH_CORRECTABLE_FAULT             ((2 << 6) | (2 << 4) | 5)
#define MESH_TABLE_LOAD_FAILED             ((2 << 6) | (2 << 4) | 6)

/*
[Event Data2]
LSB of Mesh Node ID
*/

/*
[Event Data3]
MSB of Mesh Node ID
*/

#define MESH_SEL_SENSOR_TYPE       (0xDC)
#define MESH_SEL_SENSOR_NUM_ECC    (0xBD)
#define MESH_SEL_EVENT_DIR_TYPE    (0x70)
#define MESH_SEL_FAULT_OFFSET      (0x03)
#define MESH_SEL_NODE_ID_MASK_LSB  (0xFF)
#define MESH_SEL_NODE_ID_MASK_MSB  (0xFF00)
#define MESH_SEL_NODE_ID_MSB_SHIFT (8)

// vendor_specific_data[0] - Error Node
// vendor_specific_data[1] - Error Type
// vendor_specific_data[2] - Error Index Offset
// vendor_specific_data[3] - Error Index Address

#define MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_0 (0)  // Error Node
#define MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_1 (1)  // Error Type
#define MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_2 (2)  // Error Index Offset
#define MESH_RAS_VENDOR_SPECIFIC_DATA_OFFSET_3 (3)  // Error Index Address

#define NUM_OF_CCG_WITH_D2D 8

#define D2DSS_TEST_RAS_DUMMY_ADDRESS            (0x20080001234ULL)
#define D2DSS_TEST_RAS_INJ_COUNTER              (0xFFFFF)
#define D2DSS_TEST_RAS_ERROR_INF                (0x102000)

/**
 * Mesh Fault ISR
 * This function is called when a Mesh Fault ISR is triggered by the hardware INT
 * @param context
 * @return void
 **/
void mesh_fault_isr(void* context);

/**
 * Mesh Error ISR
 * This function is called when a Mesh Error ISR is triggered by the hardware INT
 * @param context
 * @return void
 **/
void mesh_error_isr(void* context);

/**
 * D2D Error ISR
 * This function is called when a D2D Error ISR is triggered by the hardware INT
 * @param context
 * @return void
 **/
void d2d_error_isr(void* context);

/**
 * D2D RAS Initialization
 * This function initializes the D2D RAS Agents
 * @param void
 * @return void
 **/
void d2d_ras_init(void);

/**
 * API to inject error in D2D subsystem
 * @param d2d_subsystem D2D subsystem number
 * @param err_inj Error injection value
 * @param err_cnt_down Error count down value
 * @return void
 **/
void d2d_ras_error_inj(uint8_t d2d_subsystem, uint32_t err_inj, uint32_t err_cnt_down);
