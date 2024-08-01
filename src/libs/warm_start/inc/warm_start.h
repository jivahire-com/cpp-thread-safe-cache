//
// Copyright (c) Microsoft Corporation.
//

/**
 * @file mod_warm_start.h
 * Warm start data access module public header
 */

#pragma once

/*----------- Nested includes ------------*/
#include "warm_start_id.h"
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

typedef struct _config_warm_start_t {
    void *p_base;
    uint32_t size;
} config_warm_start_t;

/*--------- Function Prototypes ----------*/

/**
 *  API to get the location and size of the data.
 *  If there is no matching data or if the data is
 *  invalid, this api shall return NULL and the p_size
 *  shall be set to 0.
 * 
 *  Note** To not call ws_data_get from an ISR
 *
 *  @param id
 *      Warm data namespace assigned ID
 *
 *  @param p_size
 *      Location that will hold the data block size (in bytes)
 *
 *  @return
 *      void *  - location of the saved data.  Note: this pointer is valid
 *                for the life of the data meaning as long as the size doesn't
 *                change when calling ws_data_put, this data location won't change
 *                and the user is able to use this space as desired.
 *      NULL    - Data could not be found or is invalid or invalid input
 */
void *ws_data_get(mod_ws_data_id_t id, uint32_t *p_size);

/**
 *  API used to place data into reserved non-initialized
 *  memory space. Note: Size = 0 will
 *  erase an entry.
 *  
 *  Note** To not call ws_data_put from an ISR
 *
 *  @param id
 *      Warm data namespace assigned ID
 *
 *  @param p_data
 *      Pointer to the data to save (p_data == NULL allocs an entry only)
 *
 *  @param size
 *      Size of the  data to save
 *
 *  @return
 *      void * - A pointer to where the data is placed.  The location of the data will
 *               not change over the life of the data which means as long as the size of
 *               the data doesn't change, the pointer can be used to directly store fields
 *               in the data if the consumer chooses.  Calling this function with the same
 *               size data will not change the data location.
 *      NULL -   Data could not be saved.  Note: if this module is not able to store the data,
 *               the module will assert.
 */
void *ws_data_put(mod_ws_data_id_t id, void *p_data, uint32_t size);

/**
 *  API used to initialize the ws_list and total size of the warm start data in rmss memory
 */
void warm_start_init( void );

