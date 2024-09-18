//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file atu_api.h
 * Header containing definitions for the atu module driver interface.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>
#include <DfwkTypes.h>
#include <DfwkCommon.h>
#include <in_band_telemetry_ddr.h>
#include <kng_soc_constants.h>
#include <silibs_ap_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/


/**
 * 1. All addresses, including the addresses in the AP Space, must be aligned to the ATU page size.
 * 
 * 2. All base and end addresses are inclusive, meaning the end address is the last address accessible
 *    addresses in the range. For example, if the base address is 0x1000 and the size is 0x1000, the
 *    end address is 0x1FFF.
 * 
 * 3. The mapped addresses in AP Window for the MSCP are back to back. Meaning that the next window
 *    starts at the next ATU Page aligned address after the end address of the previous window.
 * 
 * 4. The below mappings into the AP Window are the final static mappings applied for the MSCP. Any
 *    mappings initialized before the ATU Service will be overridden by these mappings, therefore any
 *    runtime dependencies on ATU mappings should depend on the ATU Service, and utilize these maps.
 */

// The SCP and MCP have the same start address for the AP Window, so we can use the SCP's address
#define MSCP_ATU_AP_WINDOW_SIZE (SCP_TOP_ATU_AP_WINDOW_MEM_SIZE)
#define MSCP_ATU_AP_WINDOW_BASE_ADDRESS (SCP_TOP_ATU_AP_WINDOW_MEM_ADDRESS)
#define MSCP_ATU_AP_WINDOW_END_ADDRESS (MSCP_ATU_AP_WINDOW_BASE_ADDRESS + MSCP_ATU_AP_WINDOW_SIZE)

// We map the ARSM on both DIEs into the window
#define MSCP_ATU_AP_WINDOW_ARSM_DIE_0_SIZE (AP_TOP_D0_ARSM_SHARED_SRAM_SIZE)
#define MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR (MSCP_ATU_AP_WINDOW_BASE_ADDRESS)
#define MSCP_ATU_AP_WINDOW_ARSM_DIE_0_END_ADDR ALIGN_UP(MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + MSCP_ATU_AP_WINDOW_ARSM_DIE_0_SIZE, ATU_PAGE_SIZE) - 1

#define MSCP_ATU_AP_WINDOW_ARSM_DIE_1_SIZE (AP_TOP_D1_ARSM_SHARED_SRAM_SIZE)
#define MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR (MSCP_ATU_AP_WINDOW_ARSM_DIE_0_END_ADDR + 1)
#define MSCP_ATU_AP_WINDOW_ARSM_DIE_1_END_ADDR ALIGN_UP(MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + MSCP_ATU_AP_WINDOW_ARSM_DIE_1_SIZE, ATU_PAGE_SIZE) - 1

// We only need to map the core cluster on the die into the window, and the cluster size is the same on each die
#define MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_SIZE (AP_TOP_D0_CORE_CLUSTER_SIZE)
#define MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR (MSCP_ATU_AP_WINDOW_ARSM_DIE_1_END_ADDR + 1)
#define MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_END_ADDR ALIGN_UP(MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR + MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_SIZE, ATU_PAGE_SIZE) - 1

// Each die has a unique DDR range for IB Telemetry
// TODO: Do we want to only map this if on the MCP?
#define MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_SIZE (IB_TELEMETRY_DDR_DIE_0_SIZE)
#define MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR (MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_END_ADDR + 1)
#define MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_END_ADDR ALIGN_UP(MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR + MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_SIZE, ATU_PAGE_SIZE) - 1

#define ATU_MODULE_NAME "[ATU_SVC] "
#ifndef NEWLINE
#define NEWLINE "\n"
#endif

#define ATU_LOG_INFO(fmt, ...) printf(ATU_MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define ATU_LOG_WARN(fmt, ...) printf(ATU_MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define ATU_LOG_CRIT(fmt, ...) printf(ATU_MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

/*-------------- Typedefs ----------------*/
/**
 *  @brief ATU service interface type
 *
 *    ATU module will provide a read/write type through the interface to the clients.
 *
 */
typedef enum _atu_api_type
{
    ATU_IO_REQUEST_MAP_SYNC,
    ATU_IO_REQUEST_UNMAP_SYNC,
} atu_api_type_t;

// struct for the atu device object
typedef struct {
    DFWK_DEVICE_HEADER header;
    DFWK_QUEUE default_queue;
} atu_device_t;

// struct for an interface to the atu service
typedef struct  {
    atu_device_t* atu_device;
    DFWK_INTERFACE_HEADER header;
} atu_service_t;

// struct for an request to the atu service
typedef struct {
    DFWK_SYNC_REQUEST_HEADER header;
} atu_service_request_t;
