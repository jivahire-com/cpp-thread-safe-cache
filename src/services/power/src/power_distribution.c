//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_distribution.c
 * Implements the resource distribution detail for the power service
 */

/*------------- Includes -----------------*/
#include "power_distribution_i.h"
#include "power_events.h"
#include "power_i.h"
#include "power_loops_i.h"
#include "power_stub_i.h"

#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <debug.h>
#include <dvfs_struct.h>
#include <inttypes.h>
#include <scf_power.h> // for C_STATE enums
#include <sensor_fifo_service.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
uint8_t power_distribution_get_minimum_plimit(power_runconfig_t* p_runconfig, power_cores_t* p_cores, unsigned int core)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);
    FPFW_RUNTIME_ASSERT(p_cores != NULL);
    FPFW_RUNTIME_ASSERT(core < NUM_AP_CORES_PER_DIE);

    // starting min is the greater of allowed minimum or the minimum associated with the assigned VFT
    uint8_t core_min_plimit = MAX(p_runconfig->knobs.allowed_plimit_minimum,
                                  p_runconfig->derived.vfts[p_runconfig->derived.assigned_vft[core]].min_plimit);

    const uint8_t pnominal = p_runconfig->derived.pnominal;

    // consider C state
    if (pnominal > core_min_plimit)
    {
        if (((p_cores->core[core].current_cstate == CSTATE_4) && (p_runconfig->knobs.c4_cores_limit_to_nominal)) ||
            ((p_cores->core[core].current_cstate == CSTATE_3) && (p_runconfig->knobs.c3_cores_limit_to_nominal)) ||
            ((p_cores->core[core].current_cstate == CSTATE_2) && (p_runconfig->knobs.c2_cores_limit_to_nominal)))
        {
            core_min_plimit = pnominal;
        }
    }
    // consider OS desired request (remember lower plimits are higher perf)
    uint8_t core_desired = p_cores->core[core].desired_pstate;
    // if we're not allowing plimit below nominal in the non-capping
    // case, adjust our OS view of desired to bottom out at nominal
    if ((!p_runconfig->knobs.allow_plimit_below_nominal) && (core_desired > pnominal))
    {
        core_desired = pnominal;
    }
    if (p_cores->core[core].desired_pstate_in_use >= core_desired)
    {
        // automatically bump up in_use value and/or reset counter, etc,
        // if the desired is raised above our in_use value
        p_cores->core[core].desired_pstate_in_use = core_desired;
        // clear desired count
        p_cores->core[core].desired_pstate_count = 0;
    }
    else
    {
        if ((p_cores->core[core].desired_pstate_count == 0) || (core_desired < p_cores->core[core].desired_pstate_for_count))
        {
            // here we'd have a performance request increase which
            // is still less than the in_use value (or primed value of
            // MAX_PLIMIT), so bump up the value we'll move to if the count
            // expires
            p_cores->core[core].desired_pstate_for_count = core_desired;
        }
        if (p_cores->core[core].desired_pstate_count == p_runconfig->knobs.intervals_to_lower_plimit)
        {
            // if count expired, set in_use to pstate for count expiration
            p_cores->core[core].desired_pstate_in_use = p_cores->core[core].desired_pstate_for_count;
            p_cores->core[core].desired_pstate_count = 0;
            if (core_desired > p_cores->core[core].desired_pstate_in_use)
            {
                // if there's a desired perf drop when previous one is
                // handled, start count over to move towards the new
                // drop
                p_cores->core[core].desired_pstate_for_count = core_desired;
                ++p_cores->core[core].desired_pstate_count;
            }
        }
        else
        {
            ++p_cores->core[core].desired_pstate_count;
        }
    }

    // apply OS desired to current limit
    if (p_cores->core[core].desired_pstate_in_use > core_min_plimit)
    {
        core_min_plimit = p_cores->core[core].desired_pstate_in_use;
    }

    // now apply max step sizes
    if (core_min_plimit > p_cores->core[core].current_plimit)
    {
        // this is a step down
        const uint8_t step_max = p_cores->core[core].current_plimit + p_runconfig->knobs.max_plimit_step_size_down;
        if (core_min_plimit > step_max)
        {
            core_min_plimit = step_max;
        }
    }
    else
    {
        // this is a step up
        uint8_t step_min = p_cores->core[core].current_plimit - p_runconfig->knobs.max_plimit_step_size_up;
        // don't need this check above, but if max steps > number of
        // steps to 0, the uint will wrap back to max value, so handle
        // that here
        if (step_min > MAX_PLIMIT)
        {
            step_min = 0;
        }
        if (core_min_plimit < step_min)
        {
            core_min_plimit = step_min;
        }
    }

    return core_min_plimit;
}

/**
 * @brief Select a plimit for a core
 *
 */
static uint8_t select_plimit(power_runconfig_t* p_runconfig,
                             power_ctrl_loop_detail_t* p_ctrlloop,
                             unsigned int core,
                             uint8_t plimit,
                             uint8_t max_base)
{
    power_cores_t* p_cores = &p_ctrlloop->cores;

    // resource distribution is done based on bitfields rather than looking up
    // details about each individual core; so selected plimit needs to be

    // limit to base perf if not boosted
    const uint8_t base_pstate = p_cores->core[core].current_base_pstate;
    // if we're in the window between nominal and base, we have to use our base perf as the limit, unless our max_base has been increased
    if ((plimit >= p_runconfig->derived.pnominal) && (plimit < base_pstate))
    {
        plimit = MIN(max_base, p_cores->core[core].current_base_pstate);
    }
    // adjusted to min determined previously
    plimit = MAX(p_cores->core[core].min_plimit, plimit);
    // and limit to maximum/lowest perf
    plimit = MIN(p_runconfig->knobs.allowed_plimit_maximum, plimit);

    // store selected plimit (used for vcpu calculation, etc)
    p_cores->core[core].selected_plimit = plimit;
    // set pending only if current limit differs from selection
    if (p_cores->core[core].current_plimit != plimit)
    {
        corebits_set_bit(&p_ctrlloop->plimits_pending, core);
    }
    return plimit;
}

/**
 * @brief Distribute nominal based on available resources
 *
 * @param[in] p_ctrlloop - Pointer to control loop data structure
 * @param[in] nominal_plimit - The first plimit which should be distributed for
 * nominal
 * @param[inout] p_per_pri_selections - Plimit selections per throttling priority
 * (array)
 * @param[out] distributed_plimit - Max distributed plimit (may be
 * nominal_plimit or worst)
 *
 * @return resources required for selections
 *
 */
static uint16_t power_distribution_distribute_nominal(power_ctrl_loop_detail_t* p_ctrlloop,
                                                      unsigned nominal_plimit,
                                                      unsigned* p_per_pri_selections,
                                                      unsigned* distributed_plimit)
{
    uint16_t total_required_resources = 0;
    *distributed_plimit = nominal_plimit;

    // loop through all throttling priorities attempting to distribute nominal performance
    for (unsigned throttle_pri_idx = 0; throttle_pri_idx < VM_THROT_COUNT; ++throttle_pri_idx)
    {
        // we track the last distributed plimit for each priority, a lower priority can not be given a higher performance that previous priority
        const unsigned last_distributed = *distributed_plimit;
        // find the number of cores at this priority level on local and remote core
        const unsigned total_cores_at_pri = (p_ctrlloop->local.pri_counts.throt_pri_count[throttle_pri_idx] +
                                             p_ctrlloop->remote.pri_counts.throt_pri_count[throttle_pri_idx]);
        // track how many cores we have found with alternative base nominal perf levels as we climb from MAX_PLIMIT up to nominal
        unsigned found_count_in_pri = 0;
        // track the number of resources required so far to achieve nominal; only the found cores are included
        unsigned found_required_resources = 0;
        unsigned previous_required_for_remaining_cores = 0;

        if (total_cores_at_pri == 0)
        {
            // if there are no cores ar this priority, just assign the last distributed plimit to this priority
            p_per_pri_selections[throttle_pri_idx] = last_distributed;
            continue;
        }

        // search from MAX_PLIMIT up to nominal (or the last achieved plimit) to find the plimit that can be distributed to all cores at this priority
        for (unsigned plimit_idx = MAX_PLIMIT; plimit_idx >= last_distributed; plimit_idx--)
        {
            unsigned cores_remaining = total_cores_at_pri - found_count_in_pri;
            unsigned required = PLIMIT_TO_RESOURCES(plimit_idx); // the number of resources per-core required to achieve this plimit

            // for cores_remaining (cores we haven't previously applied their base nominal requirement to found_required_resources), do we have enough resources to allow this plimit?
            if (((required * cores_remaining) + found_required_resources + total_required_resources) >
                p_ctrlloop->curr_resources)
            {
                // no - previous iteration plimit is being selected for remaining cores--update required/found resources
                found_required_resources += (previous_required_for_remaining_cores * cores_remaining);
                break;
            }

            // store this iteration's requirement for possible use in the next iteration
            previous_required_for_remaining_cores = required;

            // yes - find the number of cores that have their base perf (nominal) at this plimit (local and remote)
            const unsigned cores_with_nominal_at_this_plimit =
                (p_ctrlloop->local.pri_counts.required_nominal_for_throttle[plimit_idx][throttle_pri_idx] +
                 p_ctrlloop->remote.pri_counts.required_nominal_for_throttle[plimit_idx][throttle_pri_idx]);

            // update the found requirement for cores with nominal at this plimit
            found_required_resources += (required * cores_with_nominal_at_this_plimit);
            found_count_in_pri += cores_with_nominal_at_this_plimit;

            // update the last distributed plimit and the current priority selection
            *distributed_plimit = plimit_idx;
            p_per_pri_selections[throttle_pri_idx] = plimit_idx;
        }

        // update total required resources
        total_required_resources += found_required_resources;
    }

    return total_required_resources;
}

/**
 * @brief Distribute turbo based on available resources
 *
 * @param[in] p_ctrlloop - Pointer to control loop data structure
 * @param[in] nominal_required_resources - Number of resources which are already required to achieve nominal
 * @param[in] first_turbo_plimit - The first turbo plimit which should be attempted to distribute
 * @param[inout] p_per_pri_selections - Plimit selections per throttling priority (array)
 * @param[inout] p_max_base - Maximum base plimit for each boost level
 *
 */
static void power_distribution_distribute_turbo(power_ctrl_loop_detail_t* p_ctrlloop,
                                                uint16_t nominal_required_resources,
                                                unsigned first_turbo_plimit,
                                                unsigned* p_per_pri_selections,
                                                unsigned* p_max_base)
{
    uint16_t total_required_resources = nominal_required_resources;
    bool exhausted = false;

    for (unsigned boost_pri_idx = 0; boost_pri_idx < VM_BOOST_COUNT && (!exhausted); ++boost_pri_idx)
    {
        // initialize found count of cores that have their base perf at a lower performance level
        unsigned found_count_in_pri = 0;

        // first loop is to bring all cores in this priority to nominal (cores that had base perf lower than nominal)
        for (unsigned plimit_idx = MAX_PLIMIT - 1; (plimit_idx > first_turbo_plimit) && (!exhausted); plimit_idx--)
        {
            unsigned required_delta = PLIMIT_TO_RESOURCES(plimit_idx) - PLIMIT_TO_RESOURCES(plimit_idx + 1);

            // nominal and below entries in the required_for_boost table indicate cores with nominal at a particular plimit
            const unsigned cores_with_nominal_at_prev_plimit =
                (p_ctrlloop->local.pri_counts.required_for_boost[plimit_idx + 1][boost_pri_idx] +
                 p_ctrlloop->remote.pri_counts.required_for_boost[plimit_idx + 1][boost_pri_idx]);

            // update found count to increase to this performance level
            found_count_in_pri += cores_with_nominal_at_prev_plimit;

            // cores with nominal at this plimit have already been accounted for in the nominal distribution, so we only need to account for cores found in previous iterations
            const unsigned required_resources = (required_delta * found_count_in_pri);
            if ((required_resources + total_required_resources) > p_ctrlloop->curr_resources)
            {
                exhausted = true;
                break;
            }

            // if we could achieve this plimit, update the total required resources
            total_required_resources += required_resources;

            // since the selected plimit is already nominal, we are updating the max base perf for this boost level
            // -- a core that has base perf lower than nominal can raise its perf to this new level
            p_max_base[boost_pri_idx] = plimit_idx;
        }

        // second loop is to find boost level
        for (int plimit_idx = first_turbo_plimit; (plimit_idx >= 0) && (!exhausted); plimit_idx--)
        {
            unsigned required_delta = PLIMIT_TO_RESOURCES(plimit_idx) - PLIMIT_TO_RESOURCES(plimit_idx + 1);
            // entries in the required_for_boost table above nominal indicate count of cores that can participate in this level of boost
            const unsigned cores_with_boost_at_this_plimit =
                (p_ctrlloop->local.pri_counts.required_for_boost[plimit_idx][boost_pri_idx] +
                 p_ctrlloop->remote.pri_counts.required_for_boost[plimit_idx][boost_pri_idx]);

            const unsigned required_resources = (required_delta * cores_with_boost_at_this_plimit);
            if ((required_resources + total_required_resources) > p_ctrlloop->curr_resources)
            {
                exhausted = true;
                break;
            }
            // update selection for this priority and required resources to achieve this boost plimit
            total_required_resources += required_resources;
            p_per_pri_selections[boost_pri_idx] = plimit_idx;
        }
    }
}

/**
 * @brief Push the selections made per throttling priority to specific cores
 *
 * @param[in] p_runconfig - Pointer to runtime configuration structure
 * @param[in] p_ctrlloop - Pointer to control loop data structure
 * @param[in] boost - Flag indicating if boost is triggered
 * @param[in] p_per_pri_selections - Plimit selections per throttling priority (array)
 * @param[in] p_max_base - Maximum base plimit for each boost level
 *
 */
static void power_distribution_push_selections(power_runconfig_t* p_runconfig,
                                               power_ctrl_loop_detail_t* p_ctrlloop,
                                               bool boost,
                                               unsigned* p_per_pri_selections,
                                               unsigned* p_max_base)
{
    /* TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1946116
                 enable power log
    power_log_thrpri_t selections = {0}; // structure to store per-priority exceptions to log
    static_assert(VM_THROT_COUNT <= DIMOF(selections.selection), "Required size mismatch in structure");
    */

    // if pstate forced, just return
    if (p_runconfig->knobs.force_pstate < NUM_PSTATES)
    {
        return;
    }

    for (unsigned pri_idx = 0; pri_idx < VM_PRI_COUNT; ++pri_idx)
    {
        const uint8_t selection = p_per_pri_selections[pri_idx];
        const uint8_t max_base = p_max_base[pri_idx];
        uint8_t min_selected_plimit = NUM_PSTATES;
        corebits_t cores_at_pri = boost ? p_ctrlloop->boost_priority[pri_idx] : p_ctrlloop->throttle_priority[pri_idx];
        POWER_LOG_TRACE("Priority group %d: P%d (%s)", pri_idx, selection, boost ? "boost" : "throttle");

        /* TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1946116
                 enable power log
        // store selection for log
        selections.selection[pri_idx] = selection;
        */

        for (int core = corebits_first(&cores_at_pri); core >= 0; core = corebits_first(&cores_at_pri))
        {
            corebits_clear_bit(&cores_at_pri, core);
            const uint8_t selected = select_plimit(p_runconfig, p_ctrlloop, core, selection, max_base);
            min_selected_plimit = MIN(selected, min_selected_plimit);
        }

        // increment selection count for this plimit selection at this priority
        if (min_selected_plimit < NUM_PSTATES)
        {
            // only increment counter if used by some core
            p_ctrlloop->plimit.selections[pri_idx].acc[min_selected_plimit]++;
        }
    }
    /* TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1946116
                 enable power log

    power_log_cores(&ALLCORES, TP_SELECTION, &(power_log_payload_t){.TP_SELECTION = selections});
    */
}

void power_distribution_distribute_resources(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* p_ctrlloop)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);
    FPFW_RUNTIME_ASSERT(p_ctrlloop != NULL);

    // this function has all the logic for taking the collected core state,
    // etc, and allocating current resources for the selection of plimits

    const uint8_t pnominal = p_runconfig->derived.pnominal;

    // selected tracks the resources per-core at the particular priority
    // level that are currently selected to assign
    unsigned selected[VM_PRI_COUNT] = {0};

    unsigned max_base[VM_BOOST_COUNT] = {NUM_PSTATES, NUM_PSTATES, NUM_PSTATES, NUM_PSTATES, NUM_PSTATES, NUM_PSTATES, NUM_PSTATES, NUM_PSTATES};
    uint16_t nominal_required_resources = 0;

    // this will be the plimit we're throttling to; expect to be NOMINAL,
    // because by default we are not throttling
    unsigned current_throttle_plimit;

    // this first loop attempts to provide all cores at all throttling
    // priorities with nominal; it's within this loop that we will determine
    // if we are throttling
    nominal_required_resources =
        power_distribution_distribute_nominal(p_ctrlloop, pnominal, selected, &current_throttle_plimit);

    // set throttling boolean based on whether we had to lower below nominal
    const bool all_at_nominal = (current_throttle_plimit == pnominal);
    p_ctrlloop->throttling = !(all_at_nominal);
    p_ctrlloop->throttle_query |= !(all_at_nominal); // shouldn't be cleared until queried

    // the block of code within this if statement will attempt to hand out
    // any turbo performance
    if ((all_at_nominal) && (nominal_required_resources < p_ctrlloop->curr_resources))
    {
        power_distribution_distribute_turbo(p_ctrlloop, nominal_required_resources, pnominal - 1, selected, max_base);
    }

    // push selections
    power_distribution_push_selections(p_runconfig, p_ctrlloop, all_at_nominal, selected, max_base);
}

void power_distribution_distribute_warmstart_resources(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* p_ctrlloop)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);
    FPFW_RUNTIME_ASSERT(p_ctrlloop != NULL);

    for (int core = 0; core < NUM_AP_CORES_PER_DIE; ++core)
    {
        select_plimit(p_runconfig, p_ctrlloop, core, MAX_PLIMIT - 1, MAX_PLIMIT); // default is one up from MAX
    }
}

void power_distribution_minimum_plimit_updates(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* p_ctrlloop)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);
    FPFW_RUNTIME_ASSERT(p_ctrlloop != NULL);

    // this function marks pending plimits on groups of cores so that all cores get marked for updates after a
    // set number of iterations, guaranteeing regular plimit updates even if the plimits aren't changing
    static unsigned iteration = 0;

    const unsigned minimum_updates = (unsigned)p_runconfig->knobs.minimum_plimit_updates;
    if (minimum_updates == power_loops_minimum_plimit_t_NONE)
    {
        // no minimum required
        return;
    }
    const unsigned count_per_iteration = 1 << (minimum_updates - 1); // minimum updates count is 2^(minimum-1)
    const unsigned required_iterations_mask = ((NUM_AP_CORES_PER_DIE + count_per_iteration - 1) / count_per_iteration);

    // loop between 0 and required_iterations
    iteration %= required_iterations_mask;

    corebits_t new_pending = {0};
    for (unsigned int core = count_per_iteration * iteration; core < count_per_iteration * (iteration + 1); ++core)
    {
        corebits_set_bit(&new_pending, core);
    }
    // log the cores included in minimum update
    /* TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1946116
                 enable power log
    power_log_cores_ts(mod_power_timer_get_counter(), &new_pending, POWER_LOG_DATA(DIST_MIN_PLIMIT_UPD, NULL));
    */
    corebits_or(&p_ctrlloop->plimits_pending, &new_pending);

    // next iteration
    iteration += 1;
}