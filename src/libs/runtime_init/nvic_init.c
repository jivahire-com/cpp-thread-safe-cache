/**
 * @file nvic_init.c
 * Initialize the NVIC
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <nvic.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(nvic)
{
    // Initialize the NVIC and reallocate the Vector Table
    nvic_init(true);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
