//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hw_fifo.h
 * Helper class for managing hardware fifos
 */

#pragma once

/*----------- Nested includes ------------*/

#include "device_fifo_id.h"
#include "sensor_fifo_driver_interface.h"
#include <fpfw_status.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

typedef struct
{
    uintptr_t  read_pointer_reg_address;
    uintptr_t  write_pointer_reg_address;
    uintptr_t  enable_reg_address;
    uintptr_t  latched_write_address;
    uint32_t   enable_mask;
    uint32_t   overflow_count;
    uint32_t   write_errors;
    uint32_t   read_errors;
    bool       enabled;
} sensor_fifo_control_t, *psensor_fifo_control_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief initialize the hw fifo's property and control tables
 *
 * @param[in] hw_fifo_prop_table - static properties of all of the fifo's
 * @param[in] hw_fifo_control - control and dynamic status of all of the fifo's
 */
void hw_fifo_init(psensor_fifo_device_properties_t hw_fifo_prop_table, psensor_fifo_control_t hw_fifo_control);

/**
 * @brief Enable the specified fifo.  Drops any stale data.
 *
 * @param[in] fifo_id - fifo identifier
 */
void hw_fifo_enable(DEVICE_FIFO_ID fifo_id);

/**
 * @brief Disable the specified fifo.
 *
 * @param[in] fifo_id - fifo identifier
 */
void hw_fifo_disable(DEVICE_FIFO_ID fifo_id);

/**
 * @brief Check if a fifo is enabled
 *
 * @param[in] fifo_id - fifo identifier
 * @retval true - enabled
 * @retval false - disabled
 */
bool hw_fifo_is_enabled(DEVICE_FIFO_ID fifo_id);

/**
 * @brief Check if there is data available
 *
 * @param[in] fifo_id - fifo identifier
 * @retval true - empty
 * @retval false - not empty
 */
bool hw_fifo_is_empty(DEVICE_FIFO_ID fifo_id);

/**
 * @brief Write an entry or entries to the fifo
 *
 * @param[in] fifo_id - fifo identifier
 * @param[in] src_data - pointer to the data that will be written
 * @param[in] entry_size - size of a single entry. Used to validate the incoming data is matched to the correct fifo.
 * @param[in] num_entries - number of entries of length entry_size that will be written.
 * @param[in] stride_index - Only pertinent on fifos with strides > entry_size.  index from the start of the current stride.
 * @retval fpfw_status_t
 */
fpfw_status_t hw_fifo_write_entry(DEVICE_FIFO_ID fifo_id, uint8_t* src_data, size_t entry_size, uint16_t num_entries, uint16_t stride_index);

/**
 * @brief Move the write pointer by the size of the stride. Only used for testing as nominally the hardware will update the write pointer per stride.
 *
 * @param[in] fifo_id - fifo identifier
 */
void hw_fifo_update_write_ptr_by_stride_size(DEVICE_FIFO_ID fifo_id);

/**
 * @brief Read an entry or entries from the fifo
 *
 * @param[in] fifo_id - fifo identifier
 * @param[in] entry_size -size of a single entry. Used to validate the incoming data is matched to the correct fifo.
 * @param[out] dest_loc - pointer to the destination
 * @param[out] num_entries_read - actual number of entries read. less than num_entries when fifo is emptied
 * @param[out] num_entries_remaining - number of entries left in the fifo. Used for control flow
 * @param[out] stride_index - index into current stride. Set to STRIDE_INDEX_UNUSED when entry size == stride size
 * @retval fpfw_status_t
 */
fpfw_status_t hw_fifo_read_entry(DEVICE_FIFO_ID fifo_id, size_t entry_size, uint8_t* dest_loc, uint16_t* num_entries_read, uint16_t* num_entries_remaining, uint16_t* stride_index);

/**
 * @brief Update read pointer.  Wrap around is handled.
 *
 * @param[in] fifo_id - fifo identifier
 * @param[in] num_entries - number of entries to add to the read pointer.
 */
void hw_fifo_update_read_ptr_by_entry_size(DEVICE_FIFO_ID fifo_id, uint16_t num_entries);

/**
 * @brief Number of entries left until the latched write pointer
 *
 * @param[in] fifo_id - fifo identifier
 * @retval size_t - number of entries
 */
size_t hw_fifo_get_remaining_latched_entries(DEVICE_FIFO_ID fifo_id);

/**
 * @brief Reads the enable from hardware for the hardware controlled fifo's and updates the shadow status.
 *
 */
void hw_fifo_get_enabled_from_hw(void);

/**
 * @brief Helper function used to update the write pointer. Can either update by entry or stride size. Determined by the
 * caller.  Wrap around is handled.
 *
 * @param[in] fifo_id - fifo identifier
 * @param[in] element_size - size of a single element
 * @param[in] num_elements - number of elements to increase write pointer by.
 */
void hw_fifo_update_write_ptr_by_size(DEVICE_FIFO_ID fifo_id, size_t element_size, uint16_t num_elements);

/**
 * @brief - Helper function for direct write. Wrap around is NOT handled, as it is handled by the caller.
 *  The write pointer is not updated because at times multiple writes are required before updating the write pointer.
 *  The write pointer is updated via hw_fifo_update_write_ptr_by_size
 *
 * @param[in] curr_write_addr - address within the fifo ram space
 * @param[in] src_data - pointer to the incoming data
 * @param[in] entry_size - size of a single entry
 * @param[in] num_entries - number of entries of size entry_size written
 * @retval fpfw_status_t
 */
fpfw_status_t hw_fifo_write_helper(uintptr_t curr_write_addr, uint8_t* src_data, size_t entry_size, uint16_t num_entries);

/**
 * @brief Helper function to determine stride index when the stride size of a fifo is > the entry size
 *
 * @param[in] fifo_id - fifo identifier
 * @param[in] fifo_read_addr - current read address used to calculate the index
 * @retval uint16_t - stride index when valid. STRIDE_INDEX_UNUSED when entry size == stride size
 */
uint16_t hw_fifo_get_stride_index(DEVICE_FIFO_ID fifo_id, uint32_t fifo_read_addr);