//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dfwk_init.c
 * Initialize the driver framework runtime.
 */

/*------------- Includes -----------------*/

#include <DfwkThreadXHost.h> // for DfwkThreadxHostInitialize, DFWK_THREADX...
#include <fpfw_init.h>       // for FPFW_INIT_COMPONENT, fpfw_init_result_t
#include <stdint.h>          // for uint32_t, uint8_t
#include <tx_api.h>          // for TX_MINIMUM_STACK, TX_NO_TIME_SLICE

/*-- Symbolic Constant Macros (defines) --*/

#define DFWK_STACK_SIZE (TX_MINIMUM_STACK + 0x1000)

#define DFWK_THREAD_PRIORITY      (1)
#define DFWK_PREEMPTION_THRESHOLD (1)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(dfwk, FPFW_INIT_DEPENDENCIES("mpu"))
{
    static DFWK_THREADX_HOST drvfwk;
    static uint8_t stack[DFWK_STACK_SIZE];

    // clang-format off
    uint32_t status = DfwkThreadxHostInitialize(
                        &drvfwk,
                        stack,
                        sizeof(stack),
                        DFWK_THREAD_PRIORITY,
                        DFWK_PREEMPTION_THRESHOLD,
                        TX_NO_TIME_SLICE
                      );
    // clang-format on

    // Enable users access to the drvfwk object. The first member being the schedule,
    // enabling users to pass the object from the init library into DfwkDeviceInitialize().
    return (fpfw_init_result_t){status, &drvfwk};
}
