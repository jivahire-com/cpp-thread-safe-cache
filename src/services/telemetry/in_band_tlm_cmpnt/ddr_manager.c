//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager.c
 * Manages DDR buffer allocation and deallocation
 * for in-band telemetry component.
 */

/*------------- Includes -----------------*/

#include "ddr_manager_i.h"
#include "in_band_tlm_cmpnt.h"
#include "in_band_tlm_cmpnt_i.h"
#include "telemetry_package_defs.h"

#include <FpFwAssert.h>
#include <fpfw_status.h>
#include <in_band_telemetry_ddr.h>
#include <stdio.h>
#include <telemetry_events_i.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

uintptr_t pwr_pkg_buffer[NUM_POWER_POOL_BLOCKS];
TX_QUEUE pwr_pkg_free_queue;

uintptr_t inst_pkg_buffer[NUM_INST_POOL_BLOCKS];
TX_QUEUE inst_pkg_free_queue;

static_assert(POWER_PKG_MAX_SIZE <= (256 * 1024), "Power package size exceeds block size");

/*------------- Functions ----------------*/
void ddr_manager_init(void)
{
    UINT status = tx_queue_create(&pwr_pkg_free_queue,
                                  "pwr_pkg_available_queue",
                                  1, // number of uint32_t elements
                                  pwr_pkg_buffer,
                                  sizeof(pwr_pkg_buffer));

    FpFwAssertWithArgs(status == TX_SUCCESS, status, (uintptr_t)&pwr_pkg_free_queue, 0, 0);

    status = tx_queue_create(&inst_pkg_free_queue,
                             "inst_pkg_available_queue",
                             1, // number of uint32_t elements
                             inst_pkg_buffer,
                             sizeof(inst_pkg_buffer));

    FpFwAssertWithArgs(status == TX_SUCCESS, status, (uintptr_t)&inst_pkg_free_queue, 0, 0);

    ddr_manager_initialize_memory();
}

void ddr_manager_init_free_memory(TX_QUEUE* queue, uintptr_t mem_start, size_t block_size, uint32_t num_blocks)
{
    UINT status = tx_queue_flush(queue);
    FpFwAssertWithArgs(status == TX_SUCCESS, status, (uintptr_t)queue, 0, 0);

    uintptr_t curr_block = mem_start;

    for (uint16_t block = 0; block < num_blocks; block++)
    {
        status = tx_queue_send(queue, &curr_block, TX_NO_WAIT);
        FpFwAssertWithArgs(status == TX_SUCCESS, status, (uintptr_t)queue, block, 0);
        curr_block += block_size;
    }
}

fpfw_status_t ddr_manager_allocate_mem_for_pwr_pkg(uintptr_t* pkg_location, size_t* available_size)
{
    fpfw_status_t status = TX_STATUS_TO_FPFW_STATUS(tx_queue_receive(&pwr_pkg_free_queue, pkg_location, TX_NO_WAIT));
    if (FPFW_STATUS_SUCCEEDED(status))
    {
        *available_size = POWER_POOL_BLOCK_SIZE;
        FPFW_ET_LOG(DdrMgrDbgAllocatePwrPkgMem, *pkg_location);
    }
    else
    {
        FPFW_ET_LOG(PwrPkgMemoryAllocationFailed, status);
    }
    return status;
}

fpfw_status_t ddr_manager_allocate_mem_for_inst_pkg(uintptr_t* pkg_location, size_t* available_size)
{
    fpfw_status_t status = TX_STATUS_TO_FPFW_STATUS(tx_queue_receive(&inst_pkg_free_queue, pkg_location, TX_NO_WAIT));
    if (FPFW_STATUS_SUCCEEDED(status))
    {
        FPFW_ET_LOG(DdrMgrDbgAllocateInstPkgMem, *pkg_location);
        *available_size = INST_POOL_BLOCK_SIZE;
    }
    else
    {
        FPFW_ET_LOG(InstPkgMemoryAllocationFailed, status);
    }
    return status;
}

void ddr_manager_allocate_mem_for_snsr_fifo_dbg_pkg(uintptr_t* pkg_location, size_t* available_size)
{
    *pkg_location = (uintptr_t)IB_TLM_DDR_ATU_AP_WIN_PWR_TLM_BASE_ADDR;
    *available_size = IB_TLM_DDR_ATU_AP_WIN_PWR_TLM_SIZE;
}

void ddr_manager_deallocate_mem(uintptr_t* pkg_location)
{
    TX_QUEUE* free_queue;
    UINT tx_status;
    uint32_t location = *pkg_location;

    if (IN_POWER_RANGE(location))
    {
        free_queue = &pwr_pkg_free_queue;
    }
    else if (IN_INST_RANGE(location))
    {
        free_queue = &inst_pkg_free_queue;
    }
    else
    {
        FPFW_ET_LOG(DeAllocateInvalidLocation, location);
        return;
    }

    FPFW_ET_LOG(DdrMgrDbgFreePkgMem, location);

    tx_status = tx_queue_send(free_queue, pkg_location, TX_NO_WAIT);
    if (FPFW_STATUS_FAILED(TX_STATUS_TO_FPFW_STATUS(tx_status)))
    {
        FPFW_ET_LOG(DeAllocateQueueFreeFailed, tx_status);
    }
}

void ddr_manager_initialize_memory()
{
    ddr_manager_init_free_memory(&pwr_pkg_free_queue, POWER_POOL_MEM_START, POWER_POOL_BLOCK_SIZE, NUM_POWER_POOL_BLOCKS);
    ddr_manager_init_free_memory(&inst_pkg_free_queue, INST_POOL_MEM_START, INST_POOL_BLOCK_SIZE, NUM_INST_POOL_BLOCKS);

    FPFW_ET_LOG(MtsMgrInitDDR);
}