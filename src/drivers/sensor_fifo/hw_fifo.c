//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hw_fifo.c
 * Implementation of helper hardware fifo class
 */

/*------------- Includes -----------------*/
#include "hw_fifo.h"

#include "sensor_fifo_driver_interface.h" // for (anonymous), psensor_fifo_...
#include "telemetry_defines.h"            // for QUADWORD_ADDRESS_SIZE

#include <FpFwAssert.h>        // for FPFW_RUNTIME_ASSERT
#include <fpfw_status.h>       // for fpfw_status_t, FPFW_STATUS...
#include <sensor_ram_bridge.h> // for scf_ram_read_entry, scf_ra...
#include <silibs_platform.h>   // for MMIO_READ32, MMIO_WRITE32
#include <silibs_status.h>     // for SILIBS_SUCCESS

/*-- Symbolic Constant Macros (defines) --*/
#define FIFO_TIMESTAMP_SIZE (8)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static psensor_fifo_device_properties_t s_hw_fifo_prop_table;
static psensor_fifo_control_t s_hw_fifo_control;

/*------------- Functions ----------------*/
void hw_fifo_init(psensor_fifo_device_properties_t hw_fifo_prop_table, psensor_fifo_control_t hw_fifo_control)
{
    s_hw_fifo_prop_table = hw_fifo_prop_table;
    s_hw_fifo_control = hw_fifo_control;
}

void hw_fifo_disable(DEVICE_FIFO_ID fifo_id)
{
    if (fifo_id <= LAST_HW_PROD_FIFO_ID)
    {
        MMIO_UPDATE32(s_hw_fifo_control[fifo_id].enable_reg_address, 0x0, s_hw_fifo_control[fifo_id].enable_mask);
    }

    s_hw_fifo_control[fifo_id].enabled = false;
}

void hw_fifo_enable(DEVICE_FIFO_ID fifo_id)
{
    if (hw_fifo_is_enabled(fifo_id) == false)
    {
        // clear fifo on disabled-to-enabled transition
        uint32_t read_addr = hw_fifo_get_read_address(fifo_id);
        uint32_t write_addr = hw_fifo_get_write_address(fifo_id);

        if (read_addr != write_addr)
        {
            hw_fifo_update_read_ptr(fifo_id, write_addr);
        }

        s_hw_fifo_control[fifo_id].latched_write_address = UINT32_MAX;

        if (fifo_id <= LAST_HW_PROD_FIFO_ID)
        {
            MMIO_UPDATE32(s_hw_fifo_control[fifo_id].enable_reg_address,
                          s_hw_fifo_control[fifo_id].enable_mask,
                          s_hw_fifo_control[fifo_id].enable_mask);
        }
    }
    s_hw_fifo_control[fifo_id].enabled = true;
}

bool hw_fifo_is_enabled(DEVICE_FIFO_ID fifo_id)
{
    bool enabled = false;
    if (fifo_id <= LAST_HW_PROD_FIFO_ID)
    {
        uint32_t reg_value = MMIO_READ32(s_hw_fifo_control[fifo_id].enable_reg_address);
        enabled = (reg_value & s_hw_fifo_control[fifo_id].enable_mask) == s_hw_fifo_control[fifo_id].enable_mask;
    }
    else
    {
        enabled = s_hw_fifo_control[fifo_id].enabled;
    }
    return enabled;
}

void hw_fifo_get_enabled_from_hw(void)
{
    for (DEVICE_FIFO_ID fifo_id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; fifo_id <= LAST_HW_PROD_FIFO_ID; fifo_id++)
    {
        // only update hw producer fifo's,  sw producer fifos are managed directly
        s_hw_fifo_control[fifo_id].enabled = hw_fifo_is_enabled(fifo_id);
    }
}

fpfw_status_t hw_fifo_write_entry(DEVICE_FIFO_ID fifo_id, uint8_t* src_data, size_t entry_size, uint16_t num_entries, uint16_t stride_index)
{
    FPFW_RUNTIME_ASSERT(src_data != NULL);
    FPFW_RUNTIME_ASSERT((entry_size % QUADWORD_ADDRESS_SIZE) == 0);
    FPFW_RUNTIME_ASSERT(entry_size == s_hw_fifo_prop_table[fifo_id].entry_size_bytes);
    FPFW_RUNTIME_ASSERT(num_entries > 0);

    if (hw_fifo_is_enabled(fifo_id) == false)
    {
        return FPFW_STATUS_DISABLED;
    }

    fpfw_status_t status = FPFW_STATUS_SUCCESS;
    uint16_t entries_to_copy = num_entries;
    uint32_t number_of_entries_per_stride = s_hw_fifo_prop_table[fifo_id].stride_size_bytes / entry_size;

    uintptr_t curr_fifo_write_addr = hw_fifo_get_write_address(fifo_id);

    if (number_of_entries_per_stride > 1)
    {
        // writing to current stride, no wrap around
        FPFW_RUNTIME_ASSERT(stride_index + num_entries <= number_of_entries_per_stride);
        curr_fifo_write_addr += (stride_index * entry_size);

        status = hw_fifo_write_helper(curr_fifo_write_addr, src_data, entry_size, num_entries);
        if (FPFW_STATUS_FAILED(status))
        {
            s_hw_fifo_control[fifo_id].write_errors++;
        }
    }
    else
    {
        // determine if there are enough entries to wrap around the fifo
        uint32_t entries_until_end = (s_hw_fifo_prop_table[fifo_id].end_address_excl - curr_fifo_write_addr) / entry_size;
        if (entries_until_end < entries_to_copy)
        {
            status = hw_fifo_write_helper(curr_fifo_write_addr, src_data, entry_size, entries_until_end);
            if (FPFW_STATUS_FAILED(status))
            {
                s_hw_fifo_control[fifo_id].write_errors++;
                return status;
            }
            curr_fifo_write_addr = s_hw_fifo_prop_table[fifo_id].start_address_incl;
            entries_to_copy -= entries_until_end;
            src_data += (entry_size * entries_until_end);
        }

        status = hw_fifo_write_helper(curr_fifo_write_addr, src_data, entry_size, entries_to_copy);
        if (FPFW_STATUS_FAILED(status))
        {
            s_hw_fifo_control[fifo_id].write_errors++;
            return status;
        }

        hw_fifo_update_write_ptr_by_size(fifo_id, entry_size, num_entries);
    }

    return status;
}

fpfw_status_t hw_fifo_write_helper(uintptr_t curr_write_addr, uint8_t* src_data, size_t entry_size, uint16_t num_entries)
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;
    for (uint16_t curr_entry = 1; curr_entry <= num_entries; curr_entry++)
    {
        int silibs_status =
            scf_ram_write_entry(curr_write_addr, (uintptr_t)src_data, (entry_size / QUADWORD_ADDRESS_SIZE));
        if (silibs_status == SILIBS_SUCCESS)
        {
            curr_write_addr += entry_size;
            src_data += entry_size;
        }
        else
        {
            status = FPFW_STATUS_FAIL;
            break;
        }
    }
    return status;
}

void hw_fifo_update_write_ptr_by_size(DEVICE_FIFO_ID fifo_id, size_t element_size, uint16_t num_elements)
{
    uint32_t actual_read_addr = hw_fifo_get_read_address(fifo_id);
    uint32_t actual_write_addr = hw_fifo_get_write_address(fifo_id);
    uint32_t actual_start_addr = s_hw_fifo_prop_table[fifo_id].start_address_incl;

    size_t fifo_size = s_hw_fifo_prop_table[fifo_id].end_address_excl - actual_start_addr;

    size_t remaining_size = (actual_read_addr > actual_write_addr)
                                ? (actual_read_addr - actual_write_addr)
                                : (fifo_size - (actual_write_addr - actual_read_addr));

    // handles wrap around as well as adding more elements than the buffer holds
    uint32_t next_write_offset = (actual_write_addr - actual_start_addr + (num_elements * element_size)) % fifo_size;
    uint32_t next_write_addr = actual_start_addr + next_write_offset;

    if ((num_elements * element_size) >= remaining_size)
    {
        s_hw_fifo_control[fifo_id].overflow_count++;

        uint32_t next_read_addr = next_write_addr + element_size;
        if (next_read_addr >= s_hw_fifo_prop_table[fifo_id].end_address_excl)
        {
            next_read_addr = actual_start_addr + (next_read_addr - (s_hw_fifo_prop_table[fifo_id].end_address_excl));
        }
        // overflowed,  drop oldest entries
        hw_fifo_update_read_ptr(fifo_id, next_read_addr);
    }

    hw_fifo_update_write_ptr(fifo_id, next_write_addr);
}

void hw_fifo_update_write_ptr_by_stride_size(DEVICE_FIFO_ID fifo_id)
{
    hw_fifo_update_write_ptr_by_size(fifo_id, s_hw_fifo_prop_table[fifo_id].stride_size_bytes, 1);
}

fpfw_status_t hw_fifo_read_entry(DEVICE_FIFO_ID fifo_id,
                                 size_t entry_size,
                                 uint8_t* dest_loc,
                                 uint16_t* num_entries_read,
                                 uint16_t* num_entries_remaining,
                                 uint16_t* stride_index)
{
    FPFW_RUNTIME_ASSERT(dest_loc != NULL);
    FPFW_RUNTIME_ASSERT((entry_size % QUADWORD_ADDRESS_SIZE) == 0);
    FPFW_RUNTIME_ASSERT(entry_size == s_hw_fifo_prop_table[fifo_id].entry_size_bytes);

    fpfw_status_t status = FPFW_STATUS_SUCCESS;

    *num_entries_read = 0;
    *num_entries_remaining = 0;
    *stride_index = STRIDE_INDEX_UNUSED;
    uint32_t fifo_read_addr = 0;
    int silibs_status = 0;

    if (hw_fifo_is_enabled(fifo_id) && (!hw_fifo_is_empty(fifo_id)))
    {
        if (s_hw_fifo_control[fifo_id].latched_write_address == UINT32_MAX)
        {
            // user contract is that on the first read of a poll period, the current write pointer location is
            // latched and all of the entries are read until the write pointer is reached
            s_hw_fifo_control[fifo_id].latched_write_address = hw_fifo_get_write_address(fifo_id);
        }

        fifo_read_addr = hw_fifo_get_read_address(fifo_id);
        silibs_status = scf_ram_read_entry(fifo_read_addr, (uintptr_t)dest_loc, (entry_size / QUADWORD_ADDRESS_SIZE));
        if (silibs_status == SILIBS_SUCCESS)
        {
            (*num_entries_read)++;
            hw_fifo_update_read_ptr_by_entry_size(fifo_id, 1);
            if (hw_fifo_is_empty(fifo_id))
            {
                s_hw_fifo_control[fifo_id].latched_write_address = UINT32_MAX;
            }
            *stride_index = hw_fifo_get_stride_index(fifo_id, fifo_read_addr);
        }
        else
        {
            s_hw_fifo_control[fifo_id].read_errors++;
            status = FPFW_STATUS_FAIL;
        }

        *num_entries_remaining = hw_fifo_get_remaining_latched_entries(fifo_id);
    }
    return status;
}

void hw_fifo_update_read_ptr_by_entry_size(DEVICE_FIFO_ID fifo_id, uint16_t num_entries)
{
    uint32_t actual_start_addr = s_hw_fifo_prop_table[fifo_id].start_address_incl;
    uint32_t next_read_addr = hw_fifo_get_read_address(fifo_id);

    next_read_addr += (s_hw_fifo_prop_table[fifo_id].entry_size_bytes * num_entries);

    if (next_read_addr >= s_hw_fifo_prop_table[fifo_id].end_address_excl)
    {
        next_read_addr = actual_start_addr + (next_read_addr - (s_hw_fifo_prop_table[fifo_id].end_address_excl));
    }

    hw_fifo_update_read_ptr(fifo_id, next_read_addr);
}

bool hw_fifo_is_empty(DEVICE_FIFO_ID fifo_id)
{
    bool empty = true;
    if (hw_fifo_is_enabled(fifo_id))
    {
        empty = (hw_fifo_get_read_address(fifo_id) == hw_fifo_get_write_address(fifo_id));
    }
    return empty;
}

size_t hw_fifo_get_remaining_latched_entries(DEVICE_FIFO_ID fifo_id)
{
    // Calculate the total number of bytes in the buffer
    size_t entries = 0;
    if (s_hw_fifo_control[fifo_id].latched_write_address != UINT32_MAX)
    {

        size_t buffer_size = s_hw_fifo_prop_table[fifo_id].end_address_excl - s_hw_fifo_prop_table[fifo_id].start_address_incl;
        uint32_t read_addr = hw_fifo_get_read_address(fifo_id);
        uint32_t write_addr = (uint32_t)s_hw_fifo_control[fifo_id].latched_write_address;

        // Calculate the distance in bytes from read_addr to write_addr
        size_t distance;
        if (write_addr >= read_addr)
        {
            distance = write_addr - read_addr;
        }
        else
        {
            // If write_addr has wrapped around
            distance = buffer_size - (read_addr - s_hw_fifo_prop_table[fifo_id].start_address_incl) +
                       (write_addr - s_hw_fifo_prop_table[fifo_id].start_address_incl);
        }

        // Calculate the number of entries
        entries = distance / s_hw_fifo_prop_table[fifo_id].entry_size_bytes;
    }

    return entries;
}

uint16_t hw_fifo_get_stride_index(DEVICE_FIFO_ID fifo_id, uint32_t fifo_read_addr)
{
    uint16_t stride_index = STRIDE_INDEX_UNUSED;

    uint32_t number_of_entries_per_stride =
        s_hw_fifo_prop_table[fifo_id].stride_size_bytes / s_hw_fifo_prop_table[fifo_id].entry_size_bytes;

    if (number_of_entries_per_stride > 1)
    {
        uint32_t address_diff = fifo_read_addr - s_hw_fifo_prop_table[fifo_id].start_address_incl;
        stride_index = (address_diff / s_hw_fifo_prop_table[fifo_id].entry_size_bytes) % number_of_entries_per_stride;
    }

    return stride_index;
}

uint32_t hw_fifo_get_read_address(DEVICE_FIFO_ID fifo_id)
{
    return MMIO_READ32(s_hw_fifo_control[fifo_id].read_pointer_reg_address) - FIFO_TIMESTAMP_SIZE;
}

uint32_t hw_fifo_get_write_address(DEVICE_FIFO_ID fifo_id)
{
    return MMIO_READ32(s_hw_fifo_control[fifo_id].write_pointer_reg_address) - FIFO_TIMESTAMP_SIZE;
}

void hw_fifo_update_read_ptr(DEVICE_FIFO_ID fifo_id, uint32_t address)
{
    MMIO_WRITE32(s_hw_fifo_control[fifo_id].read_pointer_reg_address, address + FIFO_TIMESTAMP_SIZE);
}

void hw_fifo_update_write_ptr(DEVICE_FIFO_ID fifo_id, uint32_t address)
{
    MMIO_WRITE32(s_hw_fifo_control[fifo_id].write_pointer_reg_address, address + FIFO_TIMESTAMP_SIZE);
}
