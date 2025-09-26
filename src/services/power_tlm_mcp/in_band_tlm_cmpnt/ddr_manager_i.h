//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_i.h
 * Private header for ddr manager used for internal access as well as unit testing
 */

#pragma once

/*----------- Nested includes ------------*/

#include <atu_api.h>
#include <fpfw_status.h>
#include <in_band_telemetry_ddr.h>
#include <tx_api.h>

#include "telemetry_package_defs.h"

/*-- Symbolic Constant Macros (defines) --*/
// these structs declared here as they are used to size the memory pools definitions below
typedef struct
{
    pwr_core_record_pstate_t pstate_record;
    pwr_core_record_cstate_t cstate_record;
    pwr_core_record_throttle_t throttle_record;
    pwr_core_record_rack_priorities_t rack_priorities_record;
    pwr_core_record_voltage_t voltage_record;
    pwr_core_record_current_t current_record;
    pwr_core_record_temperature_t temperature_record;
    pwr_core_record_power_t power_record;
    pwr_core_record_droop_count_t droop_count_record;
    pwr_soc_record_vr_rail_t vrail_record;
    pwr_soc_record_dimm_temp_t dimm_temp_record;
    pwr_soc_record_dimm_power_t dimm_power_record;
    pwr_soc_record_hnf_t hnf_record;
    pwr_soc_record_sensor_temp_t sensor_temp_record;
    pwr_soc_record_die_mesh_t die_mesh_record;
    pwr_soc_record_d2d_link_t d2d_link_record;
    pwr_soc_record_max_soc_temp_t max_soc_temp_record;
    pwr_soc_record_mpam_core_power_t mpam_core_pwr_record;
    pwr_soc_record_mpam_throttle_t mpam_throttle_record;
    pwr_soc_record_mpam_memory_power_t mpam_memory_pwr_record;
    pwr_soc_record_memory_throttle_t memory_throttle_record;
} power_full_package_t;

typedef struct
{
    pwr_core_record_histogram_t histogram_record;
    pwr_core_record_aging_t aging_record;
    pwr_soc_record_pkg_monitor_t pkg_mon_record;
} power_24_hr_package_t;

typedef struct
{
    inst_core_record_summary_t summary_record;
    inst_soc_record_rail_t rail_record;
    inst_soc_record_dimm_runtime_t dimm_runtime_record;
    inst_soc_record_die_temp_t die_temp_record;
    inst_soc_record_max_temp_t max_temp_record;
} inst_full_package_t;


#define ALIGN_TO_4KB(size)   (((size) + 4095) & ~4095)
#define ALIGN_TO_256KB(size) (((size) + 262143) & ~262143)

#define POWER_POOL_BLOCK_ALIGN(size) ALIGN_TO_256KB(size)
#define INST_POOL_BLOCK_ALIGN(size)  ALIGN_TO_4KB(size)

#define POOL_SEPARATION_SIZE (4096)

#define MIN_INST_SAMPLE_INTERVAL_MS        (5)
#define MAX_INST_NOTIFICATION_INTERVAL_MS  (100)
#define MAX_INST_SAMPLES_PER_PACKAGE       (MAX_INST_NOTIFICATION_INTERVAL_MS / MIN_INST_SAMPLE_INTERVAL_MS)
#define MAX_PENDING_PACKAGES               ((2000 / MAX_INST_NOTIFICATION_INTERVAL_MS) * 2)

#define POWER_PKG_MAX_SIZE (sizeof(power_full_package_t) + sizeof(telemetry_package_hdr_t))
#define INST_PKG_MIN_SIZE  (sizeof(inst_full_package_t) + sizeof(telemetry_package_hdr_t))
#define INST_PKG_MAX_SIZE \
    ((MAX_INST_SAMPLES_PER_PACKAGE * sizeof(inst_full_package_t)) + sizeof(telemetry_package_hdr_t))

// 24hr packages can use INST_POOL_BLOCK_SIZE blocks
#define POWER_24HR_PKG_MAX_SIZE (sizeof(power_24_hr_package_t) + sizeof(telemetry_package_hdr_t))

#define POWER_POOL_BLOCK_SIZE (POWER_POOL_BLOCK_ALIGN(POWER_PKG_MAX_SIZE))
#define INST_POOL_BLOCK_SIZE  (INST_POOL_BLOCK_ALIGN(INST_PKG_MAX_SIZE))

// power blocks are reported on the order of minutes, so only need two in flight
#define NUM_POWER_POOL_BLOCKS (2)
#define POWER_POOL_TOTAL_SIZE  (POWER_POOL_BLOCK_SIZE * NUM_POWER_POOL_BLOCKS)

// 24 hr packages aren't fully defined, likely will fit within an instantaneous block, if not will adjust
#define INST_MEM_AVAILABLE_SIZE (IB_TLM_DDR_ATU_AP_WIN_PWR_TLM_SIZE - POWER_POOL_TOTAL_SIZE - POOL_SEPARATION_SIZE)

// this is not preferred but unit testing forces this
// in the device a 64 bit DDR memory region is used, and NUM_INST_POOL_BLOCKS is calculated dynamically based on the remaining size
// The code writes inline to those DDR addresses. This can't be done in unit testing, so the base address points to the
// address of a buffer in the unit test environment. however, an address of a variable cannot be used in C to declare
// the size of an array
#ifndef IB_TLM_DDR_TEST_OVERRIDE
#define NUM_INST_POOL_BLOCKS     (INST_MEM_AVAILABLE_SIZE / INST_POOL_BLOCK_SIZE)
#else
#define NUM_INST_POOL_BLOCKS     (10)
#endif

#define INST_POOL_TOTAL_SIZE      (INST_POOL_BLOCK_SIZE * NUM_INST_POOL_BLOCKS)


#define POWER_POOL_MEM_START (IB_TLM_DDR_ATU_AP_WIN_PWR_TLM_BASE_ADDR)
#define INST_POOL_MEM_START  (POWER_POOL_MEM_START + POWER_POOL_TOTAL_SIZE + POOL_SEPARATION_SIZE)

#define IN_RANGE(address, range_start, range_size) \
    (((address) >= (range_start)) && ((address) < ((range_start) + (range_size))) ? 1 : 0)

#define IN_POWER_RANGE(address) IN_RANGE(address, POWER_POOL_MEM_START, POWER_POOL_TOTAL_SIZE)
#define IN_INST_RANGE(address)  IN_RANGE(address, INST_POOL_MEM_START, INST_POOL_TOTAL_SIZE)

#define TELEMETRY_PHYSICAL_BASE_ADDRESS (IB_TELEMETRY_RESERVATION_BASE)
#define TELEMETRY_GET_DDR_OFFSET(die_id, atu_mapped_location) ((die_id * IB_TELEMETRY_DDR_PER_DIE_SIZE) + (atu_mapped_location - MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR))

/*-------------- Typedefs ----------------*/


/*-- Declarations (Statics and globals) --*/

extern uintptr_t pwr_pkg_buffer[NUM_POWER_POOL_BLOCKS];
extern TX_QUEUE pwr_pkg_free_queue;

extern uintptr_t inst_pkg_buffer[NUM_INST_POOL_BLOCKS];
extern TX_QUEUE inst_pkg_free_queue;
/*--------- Function Prototypes ----------*/


/**
 * @brief Initialize the ddr manager
 */
void ddr_manager_init(void);

/**
 * @brief Initialize the free memory for the ddr manager
 *
 * @param[in] queue - queue to store the free memory
 * @param[in] mem_start - start of the memory
 * @param[in] block_size - size of the block
 * @param[in] num_blocks - number of blocks
 */
void ddr_manager_init_free_memory(TX_QUEUE *queue, uintptr_t mem_start, size_t block_size, uint32_t num_blocks);

/**
 * @brief Allocate memory for power package
 *
 * @param[out] pkg_location - location to store the package
 * @param[out] available_size - size of the storage location
 * @retval fpfw_status_t
 */
fpfw_status_t ddr_manager_allocate_mem_for_pwr_pkg(uintptr_t *pkg_location, size_t* available_size);

/**
 * @brief Allocate memory for instantaneous package
 *
 * @param[out] pkg_location - location to store the package
 * @param[out] available_size - size of the storage location
 * @retval fpfw_status_t
 */
fpfw_status_t ddr_manager_allocate_mem_for_inst_pkg(uintptr_t *pkg_location, size_t* available_size);

/**
 * @brief Allocate memory for sensor fifo debug package
 *
 * @param[out] pkg_location - location to store the package
 * @param[out] available_size - size of the storage location
 * @retval none
 */
void ddr_manager_allocate_mem_for_snsr_fifo_dbg_pkg(uintptr_t *pkg_location, size_t* available_size);

/**
 * @brief Deallocate memory
 *
 * @param[in] pkg_location - location to deallocate
 * @retval none
 */
void ddr_manager_deallocate_mem(uintptr_t* pkg_location);

/**
 * @brief Initialize the free memory for the ddr manager
 *
 * @retval none
 */
void ddr_manager_initialize_memory();