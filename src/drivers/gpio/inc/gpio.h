//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file gpio.h
 * GPIO public header file.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

typedef struct {
    uint32_t nvic_irq;      // NVIC IRQ number.
    uint32_t gpio_ctrl_id;  // GPIO controller ID.
} gpio_irq_config_t;

/**
 * @brief GPIO Device structure.
 * 
 */
typedef struct {
    DFWK_DEVICE_HEADER Header;
    gpio_irq_config_t* IrqConfig;   // GPIO IRQ configuration.
    uint32_t IrqConfigCount;        // Number of GPIO IRQ configurations.

    DFWK_QUEUE Queue;               // Queue for GPIO requests from clients.
    DFWK_QUEUE IsrReqQueue;         // Manual Queue for GPIO ISR requests.

    TX_THREAD GpioIsrThread;        // Thread for GPIO ISR.
    TX_QUEUE TxGpioIsrMessageQueue; // Queue for GPIO ISR messages.
} gpio_device_t, *pgpio_device_t;

/**
 * @brief GPIO Interface structure.
 * 
 */
typedef struct {
    DFWK_INTERFACE_HEADER Header;
    pgpio_device_t Device;
} gpio_interface_t, *pgpio_interface_t;

/**
 * @brief GPIO Request types.
 * 
 */
enum gpio_request_type_idx
{
    GPIO_REQUEST_NULL,      // Internal request to mark end of entry in Isr_req_queue.
    GPIO_REQUEST_ISR_ASYNC, // Request to register deferred GPIO ISR with completion callback.
    GPIO_REQUEST_COUNT
};

/**
 * @brief GPIO Request structure.
 * 
 */
typedef struct
{
    DFWK_ASYNC_REQUEST_HEADER Header;

    uint32_t status;        // Status of the GPIO request.
    uint32_t gpio_pin_id;   // GPIO pin ID/Mask to get interrupt for GPIO_REQUEST_ISR_ASYNC.
    uint32_t level;         // GPIO pin level at interrupt for GPIO_REQUEST_ISR_ASYNC.
} gpio_request_t, *pgpio_request_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize GPIO device.
 * 
 * @param dev [out] GPIO device.
 * @param schedule [in] Pointer to the schedule.
 */
void gpio_device_init(pgpio_device_t dev, PDFWK_SCHEDULE schedule);

/**
 * @brief Initialize GPIO driver interface.
 * 
 * @param iface [out] GPIO interface.
 * @param dev [in] GPIO device.
 */
void gpio_interface_init(pgpio_interface_t iface, pgpio_device_t dev);

/**
 * @brief Send Async request to GPIO driver.
 * 
 * @param request [in] GPIO request.
 */
uint32_t gpio_cmd_async(pgpio_request_t request);
