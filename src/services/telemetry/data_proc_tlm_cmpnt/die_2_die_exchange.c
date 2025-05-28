//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file die_2_die_exchange.c
 * Guarded data object for data exchange between MCP's on both dies.
 */

/*------------- Includes -----------------*/
#include "die_2_die_exchange_i.h"

#include <atu_api.h>
#include <kng_scp_tfa_shared.h>
#include <telemetry_events_i.h>

/*-- Symbolic Constant Macros (defines) --*/
#define NUMBER_OF_SECONDARY_DIES (1)
#define FIRST_SECONDARY_DIE_ID   (1)

/*------------- Typedefs -----------------*/

typedef struct
{
    uint16_t max_inst_die_temperature_dC;
} secondary_mcp_to_die0_mcp_t;

typedef struct
{
    secondary_mcp_to_die0_mcp_t sec_mcp_to_die0_mcp[NUMBER_OF_SECONDARY_DIES]; // die number - 1 to access
} die_2_die_exch_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static uint8_t this_die_id; // initialized in die_2_die_exchange_init

#ifdef PWR_TLM_TEST_OVERRIDE

    #undef MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR
static uint8_t test_array[D0_ARSM_PWR_TLM_MCP_2_MCP_SIZE];
    #define MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR (test_array)
#endif

static die_2_die_exch_t* const s_die_2_die_exchange =
    (die_2_die_exch_t*)(MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(D0_ARSM_PWR_TLM_MCP_2_MCP_BASE));

// /*------------- Functions ----------------*/

// TODO: access will be locked by a hardware semaphore,  not currently working on SVP due to a bug
// will be added via a later PR.  https://azurecsi.visualstudio.com/Dev/_workitems/edit/2585203

static_assert(sizeof(die_2_die_exch_t) <= D0_ARSM_PWR_TLM_MCP_2_MCP_SIZE,
              "die_2_die_exch_t size exceeds reserved memory region");

// TODO: atomic access for this initial data.  Later a hardware semaphore will be used to synchronize access

void die_2_die_exchange_init(uint8_t die_id)
{
    this_die_id = die_id;
}

uint8_t die_2_die_exchange_get_this_die_id(void)
{
    return this_die_id;
}

void die_2_die_exchange_write_max_die_temp(uint16_t max_die_temperature_dC)
{
    if (this_die_id < FIRST_SECONDARY_DIE_ID || this_die_id > NUMBER_OF_SECONDARY_DIES)
    {
        FPFW_ET_LOG(InvalidDieId, this_die_id);
        return;
    }

    s_die_2_die_exchange->sec_mcp_to_die0_mcp[this_die_id - 1].max_inst_die_temperature_dC = max_die_temperature_dC;
}

uint16_t die_2_die_exchange_read_max_die_temp_dC(uint8_t die_id)
{
    if (die_id < FIRST_SECONDARY_DIE_ID || die_id > NUMBER_OF_SECONDARY_DIES)
    {
        FPFW_ET_LOG(InvalidDieId, die_id);
        return 0;
    }
    uint16_t max_inst_die_temperature_dC = s_die_2_die_exchange->sec_mcp_to_die0_mcp[die_id - 1].max_inst_die_temperature_dC;
    return max_inst_die_temperature_dC;
}