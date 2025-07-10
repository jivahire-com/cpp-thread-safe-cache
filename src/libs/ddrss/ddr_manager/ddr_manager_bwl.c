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
    printf("DDR BWL+\n");
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
    printf("DDR BWL-\n");
}

bool ddr_manager_get_bwl_engaged()
{
    return s_bwlEngaged;
}

uint8_t ddr_manager_get_bwl_state()
{
    return (uint8_t)bwl_state;
}

void ddr_manager_enable_bwl_i3c()
{
    if (bwl_state == BWL_STATE_DISABLED)
    {
        ddr_manager_engage_bwl();
        printf("DDR BWL enabled by I3C\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_ENABLED_BY_I3C);
    }

    bwl_state |= BWL_STATE_ENABLED_I3C;
}

void ddr_manager_disable_bwl_i3c()
{
    bwl_state_t previous_bwl_state = bwl_state;
    bwl_state &= ~(BWL_STATE_ENABLED_I3C);

    if ((bwl_state == BWL_STATE_DISABLED) && (previous_bwl_state == BWL_STATE_ENABLED_I3C))
    {
        ddr_manager_disengage_bwl();
        printf("DDR BWL disabled by I3C\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_DISABLED_BY_I3C);
    }
}

void ddr_manager_enable_bwl_mr4()
{
    if (bwl_state == BWL_STATE_DISABLED)
    {
        ddr_manager_engage_bwl();
        printf("DDR BWL enabled by MR4\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_ENABLED_BY_MR4);
    }

    bwl_state |= BWL_STATE_ENABLED_MR4;
}

void ddr_manager_disable_bwl_mr4()
{
    bwl_state_t previous_bwl_state = bwl_state;
    bwl_state &= ~(BWL_STATE_ENABLED_MR4);

    if ((bwl_state == BWL_STATE_DISABLED) && (previous_bwl_state == BWL_STATE_ENABLED_MR4))
    {
        ddr_manager_disengage_bwl();
        printf("DDR BWL disabled by MR4\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_DISABLED_BY_MR4);
    }
}

void ddr_manager_enable_bwl_force()
{
    if (bwl_state == BWL_STATE_DISABLED)
    {
        ddr_manager_engage_bwl();
        printf("DDR BWL forced enabled\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_FORCED_ENABLE);
    }

    bwl_state |= BWL_STATE_ENABLED_FORCED;
}

void ddr_manager_disable_bwl_force()
{
    bwl_state_t previous_bwl_state = bwl_state;
    bwl_state &= ~(BWL_STATE_ENABLED_FORCED);

    if ((bwl_state == BWL_STATE_DISABLED) && (previous_bwl_state == BWL_STATE_ENABLED_FORCED))
    {
        ddr_manager_disengage_bwl();
        printf("DDR BWL forced disabled\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_FORCED_DISABLE);
    }
}
