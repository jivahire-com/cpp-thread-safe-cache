//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_bwl.c
 * This file contains the implementation for the DDR Manager BWL (bandwidth limiter) APIs
 *
 */

/*------------- Includes -----------------*/
#include "ddr_manager_bwl.h"

#include "ddr_manager_i.h"

#include <ddr_manager_events.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
// BWL State bitmask values
typedef enum
{
    BWL_STATE_DISABLED = 0,
    BWL_STATE_ENABLED_I3C = 0x1,
    BWL_STATE_ENABLED_MR4 = 0x2,
    BWL_STATE_ENABLED_FORCED = 0x4,
} bwl_state_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static bwl_state_t bwl_state = BWL_STATE_DISABLED;
static bool s_bwlEngaged = false;

/*------------- Functions ----------------*/
static void ddr_manager_engage_bwl()
{
    // Engage the DDR BWL
    // This is a stub implementation
    // Replace with actual implementation
    // ADO: #1983310
    if (s_bwlEngaged)
    {
        return;
    }

    ddr_manager_set_memhot_gpio();
    s_bwlEngaged = true;
    printf("Engaging DDR BWL\n");
}

static void ddr_manager_disengage_bwl()
{
    // Disengage the DDR BWL
    // This is a stub implementation
    // Replace with actual implementation
    // ADO: #1983310
    if (!s_bwlEngaged)
    {
        return;
    }

    ddr_manager_clear_memhot_gpio();
    s_bwlEngaged = false;
    printf("Disengaging DDR BWL\n");
}

bool ddr_manager_get_bwl_engaged()
{
    return s_bwlEngaged;
}

void ddr_manager_enable_bwl_i3c()
{
    if (bwl_state == BWL_STATE_DISABLED)
    {
        ddr_manager_engage_bwl();
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_ENABLED_BY_I3C);
    }

    bwl_state |= BWL_STATE_ENABLED_I3C;
}

void ddr_manager_disable_bwl_i3c()
{
    bwl_state &= ~(BWL_STATE_ENABLED_I3C);

    if (bwl_state == BWL_STATE_DISABLED)
    {
        ddr_manager_disengage_bwl();
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_DISABLED_BY_I3C);
    }
}

void ddr_manager_enable_bwl_mr4()
{
    if (bwl_state == BWL_STATE_DISABLED)
    {
        ddr_manager_engage_bwl();
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_ENABLED_BY_MR4);
    }

    bwl_state |= BWL_STATE_ENABLED_MR4;
}

void ddr_manager_disable_bwl_mr4()
{
    bwl_state &= ~(BWL_STATE_ENABLED_MR4);

    if (bwl_state == BWL_STATE_DISABLED)
    {
        ddr_manager_disengage_bwl();
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_DISABLED_BY_MR4);
    }
}

void ddr_manager_enable_bwl_force()
{
    if (bwl_state == BWL_STATE_DISABLED)
    {
        ddr_manager_engage_bwl();
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_FORCED_ENABLE);
    }

    bwl_state |= BWL_STATE_ENABLED_FORCED;
}

void ddr_manager_disable_bwl_force()
{
    bwl_state &= ~(BWL_STATE_ENABLED_FORCED);

    if (bwl_state == BWL_STATE_DISABLED)
    {
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_FORCED_DISABLE);
        ddr_manager_disengage_bwl();
    }
}
