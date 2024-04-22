//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file textio_pl011.h
 * pl011 uart driver that implements the fpfw_text_io_interface. This driver is based on DFWK from the 1pfw.fwlibs repo.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <fpfw_text_io_interface.h>
#include <uart_pl011.h>
#include <stddef.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/


/*-------------- Typedefs ----------------*/

typedef struct {
    uint32_t base_address;
    uint32_t interrupt;
    uint32_t baud_rate;
    uint32_t clk_freq;
    uint8_t wlen;
    uint8_t stop_bits;
    uint8_t parity;
} textio_pl011_config_t;

typedef struct {
    DFWK_DEVICE_HEADER header;
    DFWK_QUEUE default_queue;
    DFWK_QUEUE read_queue;
    DFWK_QUEUE write_queue;

    const textio_pl011_config_t *config;

    p_fpfw_text_io_async_read_request_t pending_read_request;
    p_fpfw_text_io_async_write_request_t pending_write_request;

    TX_TIMER polling_timer;
} textio_pl011_device_t;

typedef struct {
    DFWK_INTERFACE_HEADER header;
    textio_pl011_device_t *device;
} textio_pl011_interface_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * Initializes a pl011 uart device.  
 *
 * @param[in]  device
 *     A pointer to a uart device object
 *
 * @param[in]  config
 *     Config structure with settings for initializing the uart device.
 * 
 * @param[in] schedule
 *     DFWK Schedule
 */
void textio_pl011_device_initialize(textio_pl011_device_t *device, const textio_pl011_config_t *config, DFWK_SCHEDULE * schedule);


/**
 * Initializes a uart pl011 interface.   
 *
 * @param[in]  device
 *     A pointer to a uart device object
 *
 * @param[in]  pl011_interface
 *     A pointer to a uart pl011 interface structure to be initialized. This should happen before giving the interface
 *     to the client.
 */
void textio_pl011_device_interface_initialize(textio_pl011_device_t *device, textio_pl011_interface_t *pl011_interface);
