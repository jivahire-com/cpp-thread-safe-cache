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
#include <idsw_kng.h>
#include <ioss_semaphore_config.h>
#include <kng_scp_tfa_shared.h>
#include <semaphore_lib.h>
#include <telemetry_events_i.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PWR_TLM_D2D_SIZE (512)

//                                                  Ascii of 'P''T'
#define PWR_TLM_SEM_KEY(die_id) (((die_id) << 16) | (0x5054 & 0xFFFF))

/*------------- Typedefs -----------------*/

typedef struct __attribute__((packed))
{
    uint16_t ib_max_inst_die_temperature_dC;
    max_die_temps_t ib_max_pwr_pkg_die_temp;
    sliding_window_data_t oob_window_max_die_temp_dC; // sliding window for max die temperature

} secondary_mcp_to_die0_mcp_t;

typedef struct __attribute__((packed))
{
    secondary_mcp_to_die0_mcp_t sec_mcp_to_die0_mcp[NUMBER_OF_SECONDARY_DIES]; // die number - 1 to access
} die_2_die_exch_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static uint8_t this_die_id; // initialized in die_2_die_exch_init

#ifdef PWR_TLM_TEST_OVERRIDE

    #undef MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR
static uint8_t test_array[D0_ARSM_PWR_TLM_MCP_2_MCP_SIZE];
    #define MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR (test_array)
#endif

// also need mpu alignment for D0_ARSM_PWR_TLM_MCP_2_MCP_BASE,  it is of size 1k, but not on a 1k boundary.
// so round up to a 512 byte boundary as a workaround, coordinate with mpu configuration in mcp_mpu_init.c
static die_2_die_exch_t* const s_die_2_die_exch =
    (die_2_die_exch_t*)(uintptr_t)((MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR +
                                    ARSM_GET_REGION_OFFSET(ALIGN_UP(D0_ARSM_PWR_TLM_MCP_2_MCP_BASE, PWR_TLM_D2D_SIZE))));

/*------------- Functions ----------------*/

static_assert(sizeof(die_2_die_exch_t) <= PWR_TLM_D2D_SIZE /*D0_ARSM_PWR_TLM_MCP_2_MCP_SIZE*/,
              "die_2_die_exch_t size exceeds reserved memory region");

/**
 * @brief Check if the api is valid for the given die ID.
 * @param die_id The die ID to check. May either be the id of this device or the id of the api caller.
 * @return
 */
static bool die_id_is_valid(uint8_t die_id);

/**
 * @brief Zero out the fortified memory region. Required due to compiler settings since memory is located at a
 * raw memory map address. This function is used to initialize the die to die exchange memory region to zero.
 * It is called during initialization to ensure that the memory is in a known state.
 * @param ptr Pointer to the memory region to zero out.
 * @param size Size of the memory region in bytes.
 */
static void zero_fortified_memory(void* ptr, size_t size);

/**
 * @brief Wait for the semaphore to be available before accessing the die 2 die exchange.
 * This function is used to ensure that only one die accesses the exchange at a time.
 * Blocking call.
 * @param
 */
static void d2d_exch_wait_for_sem(void);

/**
 * @brief Release the semaphore after accessing the die 2 die exchange.
 * This function is used to allow other dies to access the exchange after the current die is done.
 */
static void d2d_exch_release_sem(void);

// TODO:  hardware semaphore not currently working on SVP due to a bug
// HW Semaphore bug -- https://azurecsi.visualstudio.com/1P-SoC-Modeling/_workitems/edit/2327121

static void d2d_exch_wait_for_sem(void)
{
    if (IS_PLATFORM_SVP() == false)
    {
        wait_for_semaphore(PWR_TLM_DIE_2_DIE_SEMAPHORE, PWR_TLM_SEM_KEY(this_die_id));
    }
}

static void d2d_exch_release_sem(void)
{
    if (IS_PLATFORM_SVP() == false)
    {
        release_semaphore(PWR_TLM_DIE_2_DIE_SEMAPHORE);
    }
}

void die_2_die_exch_init(uint8_t die_id)
{
    this_die_id = die_id;

    if (this_die_id == PRIMARY_DIE_ID)
    {
        if (IS_PLATFORM_SVP() == false)
        {
            initialize_semaphore(PWR_TLM_DIE_2_DIE_SEMAPHORE);
        }

        d2d_exch_wait_for_sem();

        zero_fortified_memory(s_die_2_die_exch, PWR_TLM_D2D_SIZE); // sizeof(die_2_die_exch_t));

        d2d_exch_release_sem();
    }
}

static void zero_fortified_memory(void* ptr, size_t size)
{
    memset((uint8_t*)ptr, 0, size);
}

uint8_t die_2_die_exch_get_this_die_id(void)
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

void die_2_die_exch_ib_write_inst_max_die_temp(uint16_t max_die_temperature_dC)
{
    if (die_id_is_valid(this_die_id))
    {
        // only secondary dies can write to the exchange
        d2d_exch_wait_for_sem();
        s_die_2_die_exch->sec_mcp_to_die0_mcp[this_die_id - 1].ib_max_inst_die_temperature_dC = max_die_temperature_dC;
        d2d_exch_release_sem();
    }
}

uint16_t die_2_die_exch_ib_read_inst_max_die_temp_dC(uint8_t die_id)
{
    uint16_t ib_max_inst_die_temperature_dC = 0;
    if (die_id_is_valid(die_id))
    {
        // only secondary dies values can be read from the exchange
        d2d_exch_wait_for_sem();
        ib_max_inst_die_temperature_dC = s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].ib_max_inst_die_temperature_dC;
        s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].ib_max_inst_die_temperature_dC = 0; // clear the value after reading
        d2d_exch_release_sem();
    }
    return ib_max_inst_die_temperature_dC;
}

void die_2_die_exch_ib_write_pwr_pkg_max_die_temp(uint16_t average_max_temp_dC, uint16_t num_samples, uint16_t peak_temp_dC)
{
    max_die_temps_t max_die_temperature = {average_max_temp_dC, num_samples, peak_temp_dC};

    if (die_id_is_valid(this_die_id))
    {
        // only secondary dies can write to the exchange
        d2d_exch_wait_for_sem();
        s_die_2_die_exch->sec_mcp_to_die0_mcp[this_die_id - 1].ib_max_pwr_pkg_die_temp = max_die_temperature;
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_ib_read_pwr_pkg_max_die_temp_dC(uint8_t die_id, p_max_die_temps_t max_die_temperature)
{
    if (die_id_is_valid(die_id) && max_die_temperature != NULL)
    {
        // only secondary dies values can be read from the exchange
        d2d_exch_wait_for_sem();
        *max_die_temperature = s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].ib_max_pwr_pkg_die_temp;
        memset(&s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].ib_max_pwr_pkg_die_temp,
               0,
               sizeof(max_die_temps_t)); // clear the value after reading
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_write_window_max_die_temp(uint32_t summation_dC, uint16_t num_samples)
{
    if (die_id_is_valid(this_die_id))
    {
        // only secondary dies can write to the exchange
        d2d_exch_wait_for_sem();
        s_die_2_die_exch->sec_mcp_to_die0_mcp[this_die_id - 1].oob_window_max_die_temp_dC.sum = summation_dC;
        s_die_2_die_exch->sec_mcp_to_die0_mcp[this_die_id - 1].oob_window_max_die_temp_dC.num_samples = num_samples;
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_read_window_max_die_temp(uint8_t die_id, p_sliding_window_data_t max_die_temp_window)
{
    if (die_id_is_valid(die_id) && max_die_temp_window != NULL)
    {
        // only secondary dies values can be read from the exchange
        d2d_exch_wait_for_sem();
        *max_die_temp_window = s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].oob_window_max_die_temp_dC;

        memset(&s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].oob_window_max_die_temp_dC,
               0,
               sizeof(sliding_window_data_t)); // clear the value after reading
        d2d_exch_release_sem();
    }
}
