//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file aging_counters.c
 * Update aging counter  for telemetry data
 */

/*------------- Includes -----------------*/
#include "aging_counters_i.h"
#include "compute_metrics_i.h"
#include "data_proc_tlm_cmpnt.h"
#include "data_sampling_i.h" // internal APIs
#include "data_utilities_i.h"
#include "telemetry_events_i.h"

#include <atu_api.h>
#include <core_cluster_top_regs.h> // for CORE_CLUSTER_TOP_CORE_CLUSTER0_AD...
#include <core_info.h>
#include <corebits.h>
#include <dvfs.h>
#include <pex_regs.h>
#include <stdbool.h> // for false, true
#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint8_t, uint16_t
#include <string.h>  // for memset

/*-- Symbolic Constant Macros (defines) --*/
#define AP_DVFS_CLUSTER_STRIDE \
    (CORE_CLUSTER_TOP_CORE_CLUSTER1_ADDRESS - CORE_CLUSTER_TOP_CORE_CLUSTER0_ADDRESS)

#define DVFS_CLUSTER_PEX_BASE(cluster_idx)           \
    (MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR + \
     ((CORE_CLUSTER_TOP_CORE_CLUSTER1_ADDRESS - CORE_CLUSTER_TOP_CORE_CLUSTER0_ADDRESS) * (cluster_idx)))

#define GET_AP_DVFS_BASE(cluster_idx) (DVFS_CLUSTER_PEX_BASE(cluster_idx) + PEX_DVFS_ROOT_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

aging_counter_t core_aging[NUM_AP_CORES_PER_DIE] = {0};

/*------------- Functions ----------------*/
void aging_counter_init(void)
{
    uint8_t core_id = 0;
    corebits_t* sys_cores_in_die = core_info_get_enable_cores_result();

    for (core_id = 0; core_id < NUM_AP_CORES_PER_DIE; core_id++)
    {
        const corebits_t* enabled_cores = sys_cores_in_die;
        if (!corebits_is_bit_set(enabled_cores, core_id))
        {
            continue;
        }
        aging_counter_enable_sensor_measurement(core_id, 0);
        core_aging[core_id].measurement_armed = true;
    }
}

aging_sensor_status_t aging_counter_get_sensor_status(uint8_t core_id)
{
    const uintptr_t dvfs_regs_base_addr = GET_AP_DVFS_BASE(core_id);

    if (dvfs_c2_pcm_aging_get_sensor_status(dvfs_regs_base_addr) == MEASUREMENT_COMPLETE)
    {
        return MEASUREMENT_COMPLETE;
    }
    return MEASUREMENT_NOT_COMPLETE;
}

void aging_counter_enable_sensor_measurement(uint8_t core_id, uint8_t counter_id)
{
    if (counter_id >= NUMBER_OF_AGING_COUNTER_PAIRS)
    {
        FPFW_ET_LOG(AgingCountrInValidCounterID, core_id, counter_id);
        return;
    }
    uint8_t timer_cfg = 0;
    const uintptr_t dvfs_regs_base_addr = GET_AP_DVFS_BASE(core_id);

    dvfs_c2_pcm_enable_aging_sensor_measurement(dvfs_regs_base_addr, counter_id, timer_cfg);
}

bool aging_counter_read(uint8_t core_id, uint32_t* counter_a, uint32_t* counter_b)
{
    if (core_id >= NUM_AP_CORES_PER_DIE)
    {
        FPFW_ET_LOG(AgingCounterInvalidCoreID, core_id);
        return false;
    }

    if (!core_aging[core_id].measurement_armed)
    {
        FPFW_ET_LOG(AgingCounterNotArmed, core_id);
        return false;
    }

    if (core_aging[core_id].measurement_index >= NUMBER_OF_AGING_COUNTER_PAIRS)
    {
        FPFW_ET_LOG(AgingCountrInValidCounterID, core_id, core_aging[core_id].measurement_index);
        return false;
    }

    /* Note : In the C2 PCM aging measurement mode, FW MUST ENSURE that ONLY ONE pair of ROs from the two banks
     is selected for reading by making sure that PCM_HW_CR[ro_collect] is one-hot only.
     If FW sets multiple bits of PCM_HW_CR[ro_collect], the FSM will attempt to collect data from all the specified
     ROs as supported in the regular PCM FSM mode, and this will take time to iterate over all the specified ROs. This may
     mean a higher probability of not completing the measurement since the core exits C2 in the middle of the long time
     interval required to make multiple RO measurements, and therefore not being able to get any C2 aging data.
     Section: 3.18.4.4 : ref: https://microsoft.sharepoint.com/:w:/r/teams/Kingsgate/Shared%20Documents/MicroArchitecture%20Specs/MAS/Pwr%20Mgmt%20%26%20Sensors%20MASes/Kingsgate%20Sensors%20MAS.docx?d=wfc54fea30c604d31aa28f4a8922b1218&csf=1&web=1&e=2GL5Ad
    */
    // Compute the base address of the DVFS register block
    const uintptr_t dvfs_regs_base_addr = GET_AP_DVFS_BASE(core_id);
    if (SILIBS_SUCCESS == dvfs_c2_get_pcm_bank_sensor_data(dvfs_regs_base_addr, counter_a, counter_b))
    {
        // if this is last counter for core_id, need to un-armed the counter measurement.
        if (core_aging[core_id].measurement_index == NUMBER_OF_AGING_COUNTER_PAIRS - 1)
        {
            core_aging[core_id].measurement_armed = false;
            return true;
        }

        return true;
    }
    return false;
}

void aging_counter_reset(void)
{
    uint8_t core_id = 0;
    memset(&core_aging, 0, sizeof(core_aging));

    for (core_id = 0; core_id < NUM_AP_CORES_PER_DIE; core_id++)
    {
        core_aging[core_id].measurement_armed = true;
        core_aging[core_id].measurement_index = 0;
        aging_counter_enable_sensor_measurement(core_id, 0);
    }
}
