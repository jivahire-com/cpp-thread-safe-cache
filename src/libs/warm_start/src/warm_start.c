/*
 * Copyright (c) Microsoft Corporation.
 *
 * Description:
 *     Module that support warm start
 */

/*------------- Includes -----------------*/
#include "warm_start.h"

#include "warm_start_i.h"

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <bug_check.h>
#include <fpfw_status.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tx_api.h> // for TX_MUTEX

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
ws_data_list_t* p_ws_list = NULL;
uint32_t ws_size;
static TX_MUTEX ws_data_mutex = {};

/*------------- Functions ----------------*/

/**
 *  Utility for calculating the checksum
 *
 *  @param p_data
 *      Pointer to the data
 *
 *  @param length
 *      Length in bytes of the data
 *
 *  @return
 *      checksum (32 bits 2s complement)
 */
int32_t calculate_checksum(char* p_data, uint32_t length)
{
    int32_t checksum = 0;

    if (p_data == NULL)
    {
        return 0;
    }

    for (uint32_t i = 0; i < length; i++)
    {
        checksum += (uint8_t)*p_data;
        p_data++;
    }

    checksum = ~checksum + 1;

    return checksum;
}

/**
 *  Utility for checking the validity of an entry
 *
 *  @param p_entry
 *      Pointer to the entry to validate
 *
 *  @return
 *      true    -   entry looks valid
 *      false   -   entry is invalid
 */
static bool validate_entry(ws_data_entry_t* p_entry)
{
    bool temp = ((p_entry != NULL) && (calculate_checksum((char*)&p_entry->data, p_entry->size) == p_entry->checksum));

    return temp;
}

void* ws_data_get(mod_ws_data_id_t id, uint32_t* p_size)
{
    BUG_ASSERT(p_ws_list != NULL);
    ws_data_entry_t* p_entry = &p_ws_list->entry;
    void* p_data = NULL;

    if (p_size != NULL)
    {
        WS_LOG_INFO("[WS] Data Get %d", (int)id);

        tx_mutex_get(&ws_data_mutex, TX_WAIT_FOREVER);
        *p_size = 0;

        if (p_ws_list->magic_id == WARM_START_MAGIC_ID)
        {
            // Traverse list looking for data id (Take into account structure may be corrupted)
            while ((p_entry != NULL) && ((uint32_t)p_entry < ((uint32_t)p_ws_list + ws_size)))
            {
                if ((p_entry->id == id) && (validate_entry(p_entry)))
                {
                    // Valid data
                    p_data = &p_entry->data;
                    *p_size = p_entry->size;
                    break;
                }
                p_entry = p_entry->p_next;
            }
        }
        tx_mutex_put(&ws_data_mutex);
    }

    return p_data;
}

void* ws_data_put(mod_ws_data_id_t id, void* p_data, uint32_t size)
{
    BUG_ASSERT(p_ws_list != NULL);
    ws_data_entry_t* p_entry = &p_ws_list->entry;
    void* p_entry_data = NULL;

    WS_LOG_INFO("[WS] Data put %d", (int)id);

    tx_mutex_get(&ws_data_mutex, TX_WAIT_FOREVER);

    if (p_ws_list->magic_id != WARM_START_MAGIC_ID)
    {
        // initialize list
        p_ws_list->magic_id = WARM_START_MAGIC_ID;
        p_ws_list->version = 0;
        p_entry->id = WARM_START_ID_RESERVED;
        p_entry->size = ws_size - sizeof(ws_data_list_t);
        p_entry->p_next = NULL;
    }

    // Traverse list looking for existing data or end
    while (p_entry != NULL)
    {
        if (p_entry->id == id)
        {
            if (p_entry->size == size)
            {
                // entry can be reused
                // allow checksum update only if entry address passed in
                if ((p_data != &p_entry->data) && (p_data != NULL))
                {
                    memcpy(&p_entry->data, p_data, size);
                }
                p_entry->checksum = calculate_checksum(p_data, size);
                p_entry_data = &p_entry->data;
                break;
            }
            // ID found but size doesn't match, mark as reserved
            p_entry->id = WARM_START_ID_RESERVED;
        }

        // Check if we can consolidate reserved spaces
        if ((p_entry->p_next != NULL) && (p_entry->id == WARM_START_ID_RESERVED) &&
            (p_entry->p_next->id == WARM_START_ID_RESERVED))
        {
            p_entry->size = p_entry->size + p_entry->p_next->size + sizeof(ws_data_entry_t);
            p_entry->p_next = p_entry->p_next->p_next;
        }
        p_entry = p_entry->p_next;
    }
    // Existing entry doesn't exist or size changed (note, size = 0 removes entry)
    if ((p_entry_data == NULL) && (size != 0))
    {
        p_entry = &p_ws_list->entry;
        while (p_entry != NULL)
        {
            // Check if space is available to use
            if ((p_entry->id == WARM_START_ID_RESERVED) &&
                ((p_entry->size >= size + sizeof(ws_data_entry_t)) || (p_entry->size == size)))
            {
                ws_data_entry_t* p_temp_entry = (ws_data_entry_t*)(&p_entry->data + size);
                if (p_temp_entry != p_entry->p_next)
                {
                    // Add reserved entry if there is space between new entry and next
                    p_temp_entry->id = WARM_START_ID_RESERVED;
                    p_temp_entry->size = p_entry->size - (size + sizeof(ws_data_entry_t));
                    p_temp_entry->p_next = p_entry->p_next;
                    p_entry->p_next = p_temp_entry;
                }

                // copy data to entry (p_data == NULL allocs only)
                if (p_data != NULL)
                {
                    memcpy(&p_entry->data, p_data, size);
                }
                p_entry->checksum = calculate_checksum(p_data, size);
                p_entry->size = size;
                p_entry->id = id;
                p_entry_data = &p_entry->data;
                break;
            }

            p_entry = p_entry->p_next;
        }
    }

    tx_mutex_put(&ws_data_mutex);

    // Check for error
    if (p_entry_data == NULL)
    {
        WS_LOG_ERR("[WS] No Space available %d, size %d", (int)id, (int)size);
        BUG_ASSERT(false);
    }

    return p_entry_data;
}

void warm_start_init(void)
{
    // Initialize the list attributes

    p_ws_list = (void*)SCP_WARM_START_BASE;
    ws_size = SCP_WARM_START_SIZE;

    FPFW_RUNTIME_ASSERT(tx_mutex_create(&ws_data_mutex, "ws data mutex", TX_NO_INHERIT) == TX_SUCCESS);

    WS_LOG_INFO("[WARM_START] Init done");
}