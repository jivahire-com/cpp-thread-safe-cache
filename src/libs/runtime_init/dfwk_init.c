/**
 * @file dfwk_init.c
 * Initialize the driver framework runtime.
 */

/*------------- Includes -----------------*/

#include <DfwkHost.h>
#include <DfwkThreadXHost.h>
#include <fpfw_init.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

#define DFWK_STACK_SIZE (TX_MINIMUM_STACK + 0x1000)

#define DFWK_THREAD_PRIORITY      (1)
#define DFWK_PREEMPTION_THRESHOLD (1)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(dfwk)
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

    return (fpfw_init_result_t){status, &drvfwk};
}
