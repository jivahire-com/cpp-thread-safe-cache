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
#define PWR_TLM_D2D_SIZE (512)

/*------------- Typedefs -----------------*/

typedef struct
{
    uint16_t max_inst_die_temperature_dC;
    max_die_temps_t max_pwr_pkg_die_temp;

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

// TODO: access will be locked by a hardware semaphore,  not currently working on SVP due to a bug
// will be added via a later PR.  https://azurecsi.visualstudio.com/Dev/_workitems/edit/2585203

// also need mpu alignment for D0_ARSM_PWR_TLM_MCP_2_MCP_BASE,  it is of size 1k, but not on a 1k boundary.
// so round up to a 512byte boundary as a workaround, coordinate with mpu configuration in mcp_mpu_init.c
static die_2_die_exch_t* const s_die_2_die_exchange =
    (die_2_die_exch_t*)(uintptr_t)((MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR +
                                    ARSM_GET_REGION_OFFSET(ALIGN_UP(D0_ARSM_PWR_TLM_MCP_2_MCP_BASE, PWR_TLM_D2D_SIZE))));

/*------------- Functions ----------------*/

static_assert(sizeof(die_2_die_exch_t) <= PWR_TLM_D2D_SIZE /*D0_ARSM_PWR_TLM_MCP_2_MCP_SIZE*/,
              "die_2_die_exch_t size exceeds reserved memory region");

static bool die_id_is_valid(uint8_t die_id);

static void zero_fortified_memory(void* ptr, size_t size);

// TODO: atomic access for this initial data.  Later a hardware semaphore will be used to synchronize access

void die_2_die_exchange_init(uint8_t die_id)
{
    this_die_id = die_id;
    zero_fortified_memory(s_die_2_die_exchange, PWR_TLM_D2D_SIZE); // sizeof(die_2_die_exch_t));
}

static void zero_fortified_memory(void* ptr, size_t size)
{
    memset((uint8_t*)ptr, 0, size);
}

uint8_t die_2_die_exchange_get_this_die_id(void)
{
    return this_die_id;
}

static bool die_id_is_valid(uint8_t die_id)
{
    if (die_id < FIRST_SECONDARY_DIE_ID || die_id > NUMBER_OF_SECONDARY_DIES)
    {
        FPFW_ET_LOG(InvalidDieId, die_id);
        return false;
    }
    return true;
}

void die_2_die_exchange_write_inst_max_die_temp(uint16_t max_die_temperature_dC)
{
    if (die_id_is_valid(this_die_id))
    {
        // only secondary dies can write to the exchange
        s_die_2_die_exchange->sec_mcp_to_die0_mcp[this_die_id - 1].max_inst_die_temperature_dC = max_die_temperature_dC;
    }
}

uint16_t die_2_die_exchange_read_inst_max_die_temp_dC(uint8_t die_id)
{
    uint16_t max_inst_die_temperature_dC = 0;
    if (die_id_is_valid(die_id))
    {
        // only secondary dies values can be read from the exchange
        max_inst_die_temperature_dC = s_die_2_die_exchange->sec_mcp_to_die0_mcp[die_id - 1].max_inst_die_temperature_dC;
        s_die_2_die_exchange->sec_mcp_to_die0_mcp[die_id - 1].max_inst_die_temperature_dC = 0; // clear the value after reading
    }
    return max_inst_die_temperature_dC;
}

void die_2_die_exchange_write_pwr_pkg_max_die_temp(uint16_t average_max_temp_dC, uint16_t num_samples, uint16_t peak_temp_dC)
{
    max_die_temps_t max_die_temperature = {average_max_temp_dC, num_samples, peak_temp_dC};

    if (die_id_is_valid(this_die_id))
    {
        // only secondary dies can write to the exchange
        s_die_2_die_exchange->sec_mcp_to_die0_mcp[this_die_id - 1].max_pwr_pkg_die_temp = max_die_temperature;
    }
}

void die_2_die_exchange_read_pwr_pkg_max_die_temp_dC(uint8_t die_id, p_max_die_temps_t max_die_temperature)
{
    if (die_id_is_valid(die_id) && max_die_temperature != NULL)
    {
        // only secondary dies values can be read from the exchange
        *max_die_temperature = s_die_2_die_exchange->sec_mcp_to_die0_mcp[die_id - 1].max_pwr_pkg_die_temp;
        memset(&s_die_2_die_exchange->sec_mcp_to_die0_mcp[die_id - 1].max_pwr_pkg_die_temp,
               0,
               sizeof(max_die_temps_t)); // clear the value after reading
    }
}
