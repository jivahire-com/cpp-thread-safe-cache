//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pldm_common_power.h
 * Common PLDM struct/enum for power module shared between scp & mcp
 */

#pragma once

/*----------- Nested includes ------------*/
#include <icc_mhu.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
//! index of the performance state in the composite states of the state sensor - only 1 supported composite state
#define PLDM_PERFORMANCE_COMP_STATE 0

/*-------------- Typedefs ----------------*/
//!< Power performance states
enum _pldm_performance_states
{
    PLDM_PERFORMANCE_NORMAL = 1,
    PLDM_PERFORMANCE_THROTTLED,
    PLDM_PERFORMANCE_DEGRADED,
};

//! ICC payload data format, sent by MCP to SCP to set the power cap
typedef struct
{
    icc_mhu_header_t header;
    union
    {
        struct
        {
            uint16_t power_cap; //!< Power cap in W from BMC
        } oob_input;
        struct
        {
            int32_t status;              //! Status of power cap update
            uint16_t current_power_cap;  //!< Power cap in W from Soc
            uint16_t previous_power_cap; //!< Previous power cap in W from Soc
        } soc_output;
    };
} icc_pwr_cap_request_t;

//! ICC payload data format, sent by MCP to SCP to get the performance state
typedef struct
{
    icc_mhu_header_t header;
    struct
    {
        int32_t status;
        bool throttled;
        bool degraded; //! pwr loop failure, will force pmin for both dies
    } soc_output;
} icc_pwr_throt_state_request_t;

/*--------- Function Prototypes ----------*/