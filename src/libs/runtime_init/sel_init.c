//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sel_init.c
 * This file contains the initialization for SEL (System Event Log)
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <fpfw_init.h>
#include <idhw.h>
#include <sel_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Functions ---------------*/
/**
 * @brief Initialize SEL manager queue to accept SEL events
 */
FPFW_INIT_COMPONENT(sel_mgr, FPFW_INIT_NULL_NODE)
{
    // Initialize SEL event queue
    sel_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

/**
 * @brief Register MSCP2MSCP ICC channel for SEL manager
 *
 */
FPFW_INIT_COMPONENT(sel_m2m, FPFW_INIT_DEPENDENCIES("sel_mgr", "icc_mscp2mscp"))
{
    KNG_STATUS status = sel_register_icc(SEL_ICC_MSCP2MSCP, fpfw_init_get_handle((void*)"icc_mscp2mscp"));

    if (KNG_FAILED(status))
    {
        FPFW_DBGPRINT_ERROR("SEL MSCP2MSCP ICC failed: %d\n", status);
        return (fpfw_init_result_t){status, NULL};
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

/**
 * @brief Register DIE2DIE ICC channel for SEL manager
 *
 */
FPFW_INIT_COMPONENT(sel_d2d, FPFW_INIT_DEPENDENCIES("sel_mgr", "icc_die2die"))
{
    if (!idhw_is_single_die_boot_en())
    {
        KNG_STATUS status = sel_register_icc(SEL_ICC_DIE2DIE, fpfw_init_get_handle((void*)"icc_die2die"));

        if (KNG_FAILED(status))
        {
            FPFW_DBGPRINT_ERROR("SEL D2D ICC failed: %d\n", status);
            return (fpfw_init_result_t){status, NULL};
        }
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

#if defined(MCP_RUNTIME_INIT)
/**
 * @brief Register MSCP2APS ICC channel for SEL manager
 *
 */
FPFW_INIT_COMPONENT(sel_m2as, FPFW_INIT_DEPENDENCIES("sel_mgr", "icc_mcp2aps"))
{
    KNG_STATUS status = sel_register_icc(SEL_ICC_MSCP2APS, fpfw_init_get_handle((void*)"icc_mcp2aps"));

    if (KNG_FAILED(status))
    {
        FPFW_DBGPRINT_ERROR("SEL MSCP2APS ICC failed: %d\n", status);
        return (fpfw_init_result_t){status, NULL};
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

    /**
     * @brief Register PLDM platform event ready notification for SEL manager
     *
     */
    #ifdef PLDM_DRV_WORKAROUND
FPFW_INIT_COMPONENT(sel_pldm, FPFW_INIT_DEPENDENCIES("sel_mgr", "pldm_drv"))
    #else
FPFW_INIT_COMPONENT(sel_pldm, FPFW_INIT_DEPENDENCIES("sel_mgr", "pldm"))
    #endif
{
    KNG_STATUS status = sel_register_pldm();
    if (KNG_FAILED(status))
    {
        FPFW_DBGPRINT_ERROR("SEL PLDM Init failed: %d\n", status);
        return (fpfw_init_result_t){status, NULL};
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
#endif

/**
 * @brief Initialize SEL manager DFWK interface
 *
 */
#if defined(SCP_RUNTIME_INIT)
FPFW_INIT_COMPONENT(sel_dfwk, FPFW_INIT_DEPENDENCIES("sel_mgr", "sel_m2m", "sel_d2d", "dfwk"))
#else // MCP_RUNTIME_INIT
FPFW_INIT_COMPONENT(sel_dfwk, FPFW_INIT_DEPENDENCIES("sel_mgr", "sel_m2m", "sel_d2d", "sel_pldm", "sel_m2as", "dfwk"))
#endif
{
    sel_init_dfwk_device(&((DFWK_THREADX_HOST*)fpfw_init_get_handle("dfwk"))->Schedule);

    sel_init_dfwk_interface();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

/**
 * @brief Initialize SEL manager CLI commands
 *
 */
FPFW_INIT_COMPONENT(sel_cli, FPFW_INIT_DEPENDENCIES("sel_dfwk"))
{
    // Initialize CLI
    sel_cli_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}