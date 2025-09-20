//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file die_2_die_exchange.c
 * Guarded data object for data exchange between MCP's on both dies.
 */

/*------------- Includes -----------------*/
#include "die_2_die_exchange_i.h"

#include <arm_intrinsic.h> // for __DSB
#include <atu_api.h>
#include <cmsis_m7.h>
#include <idsw_kng.h>
#include <ioss_semaphore_config.h>
#include <kng_scp_tfa_shared.h>
#include <semaphore_lib.h>
#include <telemetry_events_i.h>
#include <telemetry_package_defs.h>

/*-- Symbolic Constant Macros (defines) --*/

//                                                  Ascii of 'P''T'
#define PWR_TLM_SEM_KEY(die_id) (((die_id) << 16) | (0x5054 & 0xFFFF))

/*------------- Typedefs -----------------*/

typedef struct
{
    mpam_data_t ib_mpam_data[NUMBER_OF_MPAMS];
    max_die_temps_t ib_max_pwr_pkg_die_temp;
    uint16_t ib_max_inst_die_temperature_dC;
    sliding_window_data_t oob_window_max_die_temp_dC;  // sliding window for max die temperature
    sliding_window_data_t oob_window_soc_pwr_mW;       // sliding window SOC power
    sliding_window_data_t oob_window_max_dimm_temp_dC; // sliding window for max dimm temperature
    sliding_window_data_t oob_window_dimm_pwr_mW;      // sliding window dimm power
    sliding_window_data_t oob_window_max_vr_temp_dC;   // sliding window for max voltage rail temperature
    sliding_window_data_t oob_window_avg_pstate;       // sliding window for average pstate
    dimm_data_t oob_dimm_info[NUMBER_OF_DIMMS];        // dimm channel information

} secondary_mcp_to_die0_mcp_t;

typedef struct
{
    secondary_mcp_to_die0_mcp_t sec_mcp_to_die0_mcp[NUMBER_OF_SECONDARY_DIES]; // die number - 1 to access
} die_2_die_exch_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static uint8_t this_die_id; // initialized in die_2_die_exch_init

// Sequence tracking counters for MPAM data APIs only
static uint32_t mpam_data_reads_without_new_data_counter = 0; // incremented when read occurs with no write since last read
static uint32_t mpam_data_writes_without_consumption_counter = 0; // incremented when two writes occur without a read
static bool mpam_data_write_occurred_since_last_read = false; // tracks if write happened since last read
static bool mpam_data_read_occurred_since_last_write = true;  // tracks if read happened since last write

#ifdef PWR_TLM_TEST_OVERRIDE

    #undef MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR
static uint8_t test_array[ARSM_GET_REGION_OFFSET(D0_ARSM_PWR_TLM_MCP_2_MCP_BASE) + D0_ARSM_PWR_TLM_MCP_2_MCP_SIZE];
    #define MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR (test_array)
#endif

static die_2_die_exch_t* const s_die_2_die_exch = (die_2_die_exch_t*)(uintptr_t)((
    MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(D0_ARSM_PWR_TLM_MCP_2_MCP_BASE)));

/*------------- Functions ----------------*/

static_assert(sizeof(die_2_die_exch_t) <= D0_ARSM_PWR_TLM_MCP_2_MCP_SIZE,
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
    __DSB();

    if (IS_PLATFORM_SVP() == false)
    {
        release_semaphore(PWR_TLM_DIE_2_DIE_SEMAPHORE);
    }
}

void die_2_die_exch_init(uint8_t die_id)
{
    this_die_id = die_id;

    d2d_exch_wait_for_sem();

    if (this_die_id == PRIMARY_DIE_ID)
    {
        zero_fortified_memory(s_die_2_die_exch, sizeof(die_2_die_exch_t));
    }
    mpam_data_reads_without_new_data_counter = 0;
    mpam_data_writes_without_consumption_counter = 0;
    mpam_data_write_occurred_since_last_read = false;
    mpam_data_read_occurred_since_last_write = true;

    d2d_exch_release_sem();
}

static void zero_fortified_memory(void* ptr, size_t size)
{
    memset((uint8_t*)ptr, 0, size);
}

uint8_t die_2_die_exch_get_this_die_id(void)
{
    return this_die_id;
}

void die_2_die_exch_get_mpam_data_missed_counts(uint32_t* reads_without_new_data, uint32_t* writes_without_consumption)
{
    d2d_exch_wait_for_sem();
    if (reads_without_new_data != NULL)
    {
        *reads_without_new_data = mpam_data_reads_without_new_data_counter;
        mpam_data_reads_without_new_data_counter = 0; // clear on read
    }

    if (writes_without_consumption != NULL)
    {
        *writes_without_consumption = mpam_data_writes_without_consumption_counter;
        mpam_data_writes_without_consumption_counter = 0; // clear on read
    }
    d2d_exch_release_sem();
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
        uint16_t* temp_ptr_dC = &s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].ib_max_inst_die_temperature_dC;
        d2d_exch_wait_for_sem();
        ib_max_inst_die_temperature_dC = *temp_ptr_dC;
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
        max_die_temps_t* in_band_temp_ptr = &s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].ib_max_pwr_pkg_die_temp;
        d2d_exch_wait_for_sem();
        *max_die_temperature = *in_band_temp_ptr;
        memset(in_band_temp_ptr, 0, sizeof(max_die_temps_t)); // clear the value after reading
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_ib_write_pwr_pkg_mpam_data(mpam_data_t (*mpam_data_array)[NUMBER_OF_MPAMS])
{
    if (die_id_is_valid(this_die_id) && mpam_data_array != NULL)
    {
        d2d_exch_wait_for_sem();

        // Check for sequence violation: write when previous write hasn't been read
        if (!mpam_data_read_occurred_since_last_write)
        {
            mpam_data_writes_without_consumption_counter++;
        }
        memcpy(s_die_2_die_exch->sec_mcp_to_die0_mcp[this_die_id - 1].ib_mpam_data,
               mpam_data_array,
               sizeof(s_die_2_die_exch->sec_mcp_to_die0_mcp[0].ib_mpam_data));

        // Update sequence tracking
        mpam_data_write_occurred_since_last_read = true;
        mpam_data_read_occurred_since_last_write = false;

        d2d_exch_release_sem();
    }
}

void die_2_die_exch_ib_read_pwr_pkg_mpam_data(uint8_t die_id, mpam_data_t (*mpam_data_array)[NUMBER_OF_MPAMS])
{
    if (die_id_is_valid(die_id) && mpam_data_array != NULL)
    {
        d2d_exch_wait_for_sem();

        // Check for sequence violation: read with no write since last read
        if (!mpam_data_write_occurred_since_last_read)
        {
            mpam_data_reads_without_new_data_counter++;
        }
        memcpy(mpam_data_array,
               s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].ib_mpam_data,
               sizeof(s_die_2_die_exch->sec_mcp_to_die0_mcp[0].ib_mpam_data));

        // Update sequence tracking
        mpam_data_write_occurred_since_last_read = false;
        mpam_data_read_occurred_since_last_write = true;

        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_write_window_max_die_temp(uint32_t summation_dC, uint16_t num_samples)
{
    if (die_id_is_valid(this_die_id))
    {
        // only secondary dies can write to the exchange
        sliding_window_data_t* window_ptr = &s_die_2_die_exch->sec_mcp_to_die0_mcp[this_die_id - 1].oob_window_max_die_temp_dC;
        d2d_exch_wait_for_sem();
        window_ptr->sum = summation_dC;
        window_ptr->num_samples = num_samples;
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_read_window_max_die_temp(uint8_t die_id, p_sliding_window_data_t max_die_temp_window)
{
    if (die_id_is_valid(die_id) && max_die_temp_window != NULL)
    {
        // only secondary dies values can be read from the exchange
        sliding_window_data_t* window_ptr = &s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].oob_window_max_die_temp_dC;
        d2d_exch_wait_for_sem();
        *max_die_temp_window = *window_ptr;
        memset(window_ptr, 0, sizeof(sliding_window_data_t)); // clear the value after reading
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_write_window_soc_pwr(uint32_t summation_mW, uint16_t num_samples)
{
    if (die_id_is_valid(this_die_id))
    {
        // only secondary dies can write to the exchange
        sliding_window_data_t* soc_pwr_window_ptr =
            &s_die_2_die_exch->sec_mcp_to_die0_mcp[this_die_id - 1].oob_window_soc_pwr_mW;

        d2d_exch_wait_for_sem();
        soc_pwr_window_ptr->sum = summation_mW;
        soc_pwr_window_ptr->num_samples = num_samples;
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_read_window_soc_pwr(uint8_t die_id, p_sliding_window_data_t die_soc_pwr_window)
{
    if (die_id_is_valid(die_id) && die_soc_pwr_window != NULL)
    {
        // only secondary dies values can be read from the exchange
        sliding_window_data_t* soc_pwr_window_ptr = &s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].oob_window_soc_pwr_mW;

        d2d_exch_wait_for_sem();
        *die_soc_pwr_window = *soc_pwr_window_ptr;
        memset(soc_pwr_window_ptr, 0, sizeof(sliding_window_data_t)); // clear the value after reading
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_write_window_max_dimm_temp(uint32_t summation_dC, uint16_t num_samples)
{
    if (die_id_is_valid(this_die_id))
    {
        // only secondary dies can write to the exchange
        sliding_window_data_t* max_dimm_temp_window_ptr =
            &s_die_2_die_exch->sec_mcp_to_die0_mcp[this_die_id - 1].oob_window_max_dimm_temp_dC;

        d2d_exch_wait_for_sem();
        max_dimm_temp_window_ptr->sum = summation_dC;
        max_dimm_temp_window_ptr->num_samples = num_samples;
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_read_window_max_dimm_temp(uint8_t die_id, p_sliding_window_data_t max_dimm_temp_window)
{
    if (die_id_is_valid(die_id) && max_dimm_temp_window != NULL)
    {
        // only secondary dies values can be read from the exchange
        sliding_window_data_t* max_dimm_temp_window_ptr =
            &s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].oob_window_max_dimm_temp_dC;

        d2d_exch_wait_for_sem();
        *max_dimm_temp_window = *max_dimm_temp_window_ptr;
        memset(max_dimm_temp_window_ptr, 0, sizeof(sliding_window_data_t));
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_write_window_dimm_pwr(uint32_t summation_mW, uint16_t num_samples)
{
    if (die_id_is_valid(this_die_id))
    {
        // only secondary dies can write to the exchange
        sliding_window_data_t* dimm_pwr_window_ptr =
            &s_die_2_die_exch->sec_mcp_to_die0_mcp[this_die_id - 1].oob_window_dimm_pwr_mW;

        d2d_exch_wait_for_sem();
        dimm_pwr_window_ptr->sum = summation_mW;
        dimm_pwr_window_ptr->num_samples = num_samples;
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_read_window_dimm_pwr(uint8_t die_id, p_sliding_window_data_t die_dimm_pwr_window)
{
    if (die_id_is_valid(die_id) && die_dimm_pwr_window != NULL)
    {
        // only secondary dies values can be read from the exchange
        sliding_window_data_t* dimm_pwr_window_ptr = &s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].oob_window_dimm_pwr_mW;

        d2d_exch_wait_for_sem();
        *die_dimm_pwr_window = *dimm_pwr_window_ptr;
        memset(dimm_pwr_window_ptr, 0, sizeof(sliding_window_data_t));
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_write_window_max_vr_temp(uint32_t summation_dC, uint16_t num_samples)
{
    if (die_id_is_valid(this_die_id))
    {
        // only secondary dies can write to the exchange
        sliding_window_data_t* max_vr_temp_window_ptr =
            &s_die_2_die_exch->sec_mcp_to_die0_mcp[this_die_id - 1].oob_window_max_vr_temp_dC;

        d2d_exch_wait_for_sem();
        max_vr_temp_window_ptr->sum = summation_dC;
        max_vr_temp_window_ptr->num_samples = num_samples;
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_read_window_max_vr_temp(uint8_t die_id, p_sliding_window_data_t max_die_temp_window)
{
    if (die_id_is_valid(die_id) && max_die_temp_window != NULL)
    {
        // only secondary dies values can be read from the exchange
        sliding_window_data_t* max_vr_temp_window_ptr =
            &s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].oob_window_max_vr_temp_dC;

        d2d_exch_wait_for_sem();
        *max_die_temp_window = *max_vr_temp_window_ptr;
        memset(max_vr_temp_window_ptr, 0, sizeof(sliding_window_data_t));
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_write_window_avg_pstate(uint32_t summation, uint16_t num_samples)
{
    if (die_id_is_valid(this_die_id))
    {
        // only secondary dies can write to the exchange
        sliding_window_data_t* avg_pstate_window_ptr =
            &s_die_2_die_exch->sec_mcp_to_die0_mcp[this_die_id - 1].oob_window_avg_pstate;

        d2d_exch_wait_for_sem();
        avg_pstate_window_ptr->sum = summation;
        avg_pstate_window_ptr->num_samples = num_samples;
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_read_avg_pstate(uint8_t die_id, p_sliding_window_data_t avg_pstate_window)
{
    if (die_id_is_valid(die_id) && avg_pstate_window != NULL)
    {
        // only secondary dies values can be read from the exchange
        sliding_window_data_t* avg_pstate_window_ptr = &s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].oob_window_avg_pstate;

        d2d_exch_wait_for_sem();
        *avg_pstate_window = *avg_pstate_window_ptr;
        memset(avg_pstate_window_ptr, 0, sizeof(sliding_window_data_t)); // clear the value after reading
        d2d_exch_release_sem();
    }
}

void die_2_die_exch_oob_write_dimm_info(uint8_t channel, uint16_t average_pwr_mW, uint16_t average_temp_dC, uint16_t latest_temp_dC)
{
    if (die_id_is_valid(this_die_id) && (channel < NUMBER_OF_DIMMS))
    {
        // only secondary dies can write to the exchange
        dimm_data_t* dimm_info_ptr = &s_die_2_die_exch->sec_mcp_to_die0_mcp[this_die_id - 1].oob_dimm_info[channel];

        d2d_exch_wait_for_sem();
        dimm_info_ptr->average_pwr_mW = average_pwr_mW;
        dimm_info_ptr->average_temp_dC = average_temp_dC;
        if (latest_temp_dC > dimm_info_ptr->max_temp_dC)
        {
            dimm_info_ptr->max_temp_dC = latest_temp_dC;
        }
        d2d_exch_release_sem();
    }
}

uint16_t die_2_die_exch_oob_read_dimm_avg_temp_dC(uint8_t die_id, uint8_t channel)
{
    uint16_t avg_temp_dC = 0;
    if (die_id_is_valid(die_id) && (channel < NUMBER_OF_DIMMS))
    {
        // only secondary dies values can be read from the exchange
        dimm_data_t* dimm_info_ptr = &s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].oob_dimm_info[channel];

        d2d_exch_wait_for_sem();
        avg_temp_dC = dimm_info_ptr->average_temp_dC;
        dimm_info_ptr->average_temp_dC = 0;
        d2d_exch_release_sem();
    }
    return avg_temp_dC;
}

uint16_t die_2_die_exch_oob_read_dimm_max_temp_dC(uint8_t die_id, uint8_t channel)
{
    uint16_t max_temp_dC = 0;
    if (die_id_is_valid(die_id) && (channel < NUMBER_OF_DIMMS))
    {
        // only secondary dies values can be read from the exchange
        dimm_data_t* dimm_info_ptr = &s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].oob_dimm_info[channel];

        d2d_exch_wait_for_sem();
        max_temp_dC = dimm_info_ptr->max_temp_dC;
        dimm_info_ptr->max_temp_dC = 0;
        d2d_exch_release_sem();
    }
    return max_temp_dC;
}

uint16_t die_2_die_exch_oob_read_dimm_avg_pwr_mW(uint8_t die_id, uint8_t channel)
{
    uint16_t avg_pwr_mW = 0;
    if (die_id_is_valid(die_id) && (channel < NUMBER_OF_DIMMS))
    {
        // only secondary dies values can be read from the exchange
        dimm_data_t* dimm_info_ptr = &s_die_2_die_exch->sec_mcp_to_die0_mcp[die_id - 1].oob_dimm_info[channel];

        d2d_exch_wait_for_sem();
        avg_pwr_mW = dimm_info_ptr->average_pwr_mW;
        dimm_info_ptr->average_pwr_mW = 0;
        d2d_exch_release_sem();
    }
    return avg_pwr_mW;
}
