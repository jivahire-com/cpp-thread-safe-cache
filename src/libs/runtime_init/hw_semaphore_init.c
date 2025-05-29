/**
 * @file hw_semaphore_init.c
 * Instantiates HW semaphores.
 */

/*------------- Includes -----------------*/
#include <bug_check.h>          // for BUG_CHECK
#include <fpfw_init.h>          // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <hw_semaphore.h>       // for init_hw_semaphore
#include <silibs_platform.h>    // for DEBUG_PRINT

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
#if defined(SCP_RUNTIME_INIT)
FPFW_INIT_COMPONENT(hw_sem, FPFW_INIT_DEPENDENCIES("atu_svc", "mesh_stg_2"))
#elif defined(MCP_RUNTIME_INIT)
FPFW_INIT_COMPONENT(hw_sem, FPFW_INIT_DEPENDENCIES("atu_svc"))
#endif
{
    KNG_STATUS status = init_hw_semaphore();

    if (status != KNG_SUCCESS)
    {
        DEBUG_PRINT("HW Semaphore init Error status: [0x%x]\n", status);
        BUG_CHECK(status, 0, 0);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
