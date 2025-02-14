/*
 * Copyright (c) Microsoft Corporation.
 *
 * Description:
 *     Implementation of power vcpu calculation helper functions
 */

/*------------- Includes -----------------*/
#include "power_i.h"
#include "power_loops_i.h"
#include "power_runconfig.h"
#include "power_runconfig_i.h"

#include <bug_check.h>
#include <stddef.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
#define FREQ_MHZ_TO_HZ   (1000000.0f)
#define AF_TO_AF_PERCENT (1.0f / 100.0f)
#define C_DYN_PF_TO_F    (1.0f / 1000000000000.0f)
#define VOLT_MV_TO_V     (1.0f / 1000.0f)
#define CURRENT_MA_TO_A  (1.0f / 1000.0f)

#define FUSE_VCPU_TEMP_OFFSET  (128)
#define MAX_CURRENT_PER_CORE_A (8.0f)
/*------------- Functions ----------------*/

uint16_t power_vcpu_calc_max_core_voltage_mv(power_runconfig_t* p_runconfig, power_cores_t* p_cores)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);
    FPFW_RUNTIME_ASSERT(p_cores != NULL);

    uint16_t required_mv = 0;
    const power_service_config_t* p_config = p_runconfig->p_sconfig;
    const unsigned int core_count = p_config->platform_die_core_count;
    const power_knobs_t* p_knobs = &p_runconfig->knobs;
    const bool is_not_forced_pstate = (p_knobs->force_pstate >= NUM_PSTATES);
    // iterate over cores to determine the maximum per-core voltage requirement due to selected plimit
    for (unsigned core_idx = 0; core_idx < core_count; ++core_idx)
    {
        // determine plimit to use for this core's current calculation (MAX_PLIMIT will be the selected plimit if pstate forced)
        const uint8_t selected_plimit = is_not_forced_pstate ? p_cores->core[core_idx].selected_plimit
                                                             :     /* normal case */
                                            p_knobs->force_pstate; /* forced pstate case */
        // only want to include the valid/enabled cores
        if (corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core_idx))
        {
            const uint16_t core_mv =
                p_runconfig->derived.vfts[p_runconfig->derived.assigned_vft[core_idx]].vf[selected_plimit].voltage_mv;
            if (core_mv > required_mv)
            {
                required_mv = core_mv;
            }
        }
    }
    return required_mv;
}

float power_vcpu_calc_core_leakage_scaler(power_runconfig_t* p_runconfig, unsigned temp_dC)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    power_leakage_poly_t* leakage_poly = NULL;

    // find the coefficients for the temperature/leakage polynomial using the process corner
    uint8_t process_id = p_runconfig->fuses.process_id;
    const uint8_t unknown_process_idx = DIMOF(p_runconfig->knobs.leakage_temp_scaler.poly_coefficients) - 1;
    switch (process_id)
    {
    case PROCESS_SS:
    case PROCESS_TT:
    case PROCESS_FF:
        leakage_poly = &p_runconfig->knobs.leakage_temp_scaler.poly_coefficients[process_id - 1];
        break;
    default:
        leakage_poly = &p_runconfig->knobs.leakage_temp_scaler.poly_coefficients[unknown_process_idx];
        break;
    }

    // return the result of calculated polynomial
    const float x = ((float)temp_dC) / 10.0F;
    const float x2 = x * x;
    const float x3 = x2 * x;

    return leakage_poly->a * x3 + leakage_poly->b * x2 + leakage_poly->c * x + leakage_poly->d;
}

float power_vcpu_calc_peak_current_A(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* loop_config)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    float accum_dynamic_A = 0.0F;
    float leakage_current_A = 0.0F;

    const power_service_config_t* p_config = p_runconfig->p_sconfig;
    const unsigned int core_count = p_config->platform_die_core_count;
    const power_knobs_t* p_knobs = &p_runconfig->knobs;
    const bool is_not_forced_pstate = (p_knobs->force_pstate >= NUM_PSTATES);

    // now accumulate precalculated core dynamic and leakage current
    for (unsigned core_idx = 0; core_idx < core_count; ++core_idx)
    {
        power_core_t* core = &loop_config->cores.core[core_idx];
        // determine plimit to use for this core's current calculation (MAX_PLIMIT will be the selected plimit if pstate forced)
        const uint8_t selected_plimit = is_not_forced_pstate ? core->selected_plimit : /* normal case */
                                            p_knobs->force_pstate; /* forced pstate case */
        // only want to include valid cores
        if (corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core_idx))
        {
            const power_vft_precalc_current_t* precalc =
                &p_runconfig->precalculated_current.curveset[p_runconfig->derived.assigned_vft[core_idx]].vf[selected_plimit];
            accum_dynamic_A += precalc->dynamic;

            // get the scaler value for max temp
            const float leakage_scaler =
                power_vcpu_calc_core_leakage_scaler(p_runconfig, loop_config->cores.core->temperature_dC);
            leakage_current_A += (precalc->leakage * leakage_scaler);

            /* Updates P-Limit */
            /* Reference: "LOW VOLTAGE DROPOUT (LDO)- IP DATASHEET"
             * Link: https://microsoft.sharepoint.com/:w:/t/PowerDelivery/EUxg6yuT_MhLn9-rWxVMtp4BvtpfzgoZbQRs-ST9DADL1Q?e=1TrEOp
             * Section "Interpreting ODCM readings" states that the relationship between ODCM output and core current after trimming is
             * dout_adc(I_core) = I_core * 31.136mA.
             * This relationship depends upon the trimming condition.
             * It is assumed that the trim yields maximum ADC output when core current is 8 A.
             * In this case, the current per LSB is 32mA */

            float cur_threshold;
            cur_threshold = ((precalc->dynamic) * (p_knobs->current_threshold.t1_percent) / 100 +
                             (precalc->leakage) * leakage_scaler) *
                            255 / MAX_CURRENT_PER_CORE_A;
            core->plimit_t1 = (uint8_t)FLOAT_TO_UNSIGNED(cur_threshold);

            cur_threshold = ((precalc->dynamic) * (p_knobs->current_threshold.t2_percent) / 100 +
                             (precalc->leakage) * leakage_scaler) *
                            255 / MAX_CURRENT_PER_CORE_A;
            core->plimit_t2 = (uint8_t)FLOAT_TO_UNSIGNED(cur_threshold);

            cur_threshold = ((precalc->dynamic) * (p_knobs->current_threshold.t3_percent) / 100 +
                             (precalc->leakage) * leakage_scaler) *
                            255 / MAX_CURRENT_PER_CORE_A;
            core->plimit_t3 = (uint8_t)FLOAT_TO_UNSIGNED(cur_threshold);
        }
    }

    // return accumulated dynamic + leakage scaled by temp
    return accum_dynamic_A + leakage_current_A;
}

// input is array of points, count of points, the ldo_dac code to find a bound for, and whether this should be a lower (or upper) bound
const power_vcpu_interp_t* find_interpolation_bound(const power_vcpu_interp_t* points, unsigned count, uint16_t ldo_dac, bool find_lower_bound)
{
    FPFW_RUNTIME_ASSERT(points != NULL);

    FPFW_RUNTIME_ASSERT(count > 0);

    // intialize best_index as an invalid value to distinguish between a found index and nothing found
    unsigned best_index = count;
    unsigned lowest_value_index = count;
    unsigned highest_value_index = count;

    // no assumption is made about order of points provided
    for (unsigned point_index = 0; point_index < count; ++point_index)
    {
        // when searching for a lower bound, the input ldo code has to be above the bound
        // for an upper bound, the ldo code can be less than or equal to the bound (so only an upper bound can match an input ldo code)
        if ((find_lower_bound && (ldo_dac > points[point_index].common.ldo_dac)) ||
            (!find_lower_bound && (ldo_dac <= points[point_index].common.ldo_dac)))
        {
            // we found something; if nothing found previously or new match is closer, update best_index
            if ((best_index == count) ||
                (find_lower_bound && (points[point_index].common.ldo_dac > points[best_index].common.ldo_dac)) ||
                (!find_lower_bound && (points[point_index].common.ldo_dac < points[best_index].common.ldo_dac)))
            {
                best_index = point_index;
            }
        }
        // keep track of the lowest and highest value of all the provided points
        if ((lowest_value_index == count) ||
            (points[point_index].common.ldo_dac < points[lowest_value_index].common.ldo_dac))
        {
            lowest_value_index = point_index;
        }
        if ((highest_value_index == count) ||
            (points[point_index].common.ldo_dac > points[highest_value_index].common.ldo_dac))
        {
            highest_value_index = point_index;
        }
    }
    if (best_index != count)
    {
        return &points[best_index];
    }
    // if we didn't find a bound that fit the criteria, return either the highest/lowest point
    return (find_lower_bound ? &points[lowest_value_index] : &points[highest_value_index]);
}

// // given an upper and lower bound, calculate the interpolated value for a provided ldo_dac code
static uint32_t power_vcpu_interpolate_value(uint16_t ldo_dac,
                                             const power_vcpu_interp_t* upper_bound,
                                             const power_vcpu_interp_t* lower_bound)
{
    FPFW_RUNTIME_ASSERT(lower_bound != NULL);
    FPFW_RUNTIME_ASSERT(upper_bound != NULL);

    // the fuse field does not occupy all 32 bits, so switch to int32 to allow for negative gradient and make this more robust
    const int32_t rise = ((int32_t)upper_bound->common.field32 - (int32_t)lower_bound->common.field32);
    const int32_t run = ((int32_t)upper_bound->common.ldo_dac - (int32_t)lower_bound->common.ldo_dac);

    // if upper is a match return upper value (only the upper bound can be an exact match)
    // if either rise/run are 0, just report upper value - cases where there are no unique points or input ldo_dac is < all fused points.
    if ((upper_bound->common.ldo_dac == ldo_dac) || ((rise * run) == 0))
    {
        return upper_bound->common.field32;
    }

    float gradient = (float)rise / (float)run;
    float current = (gradient * (ldo_dac - lower_bound->common.ldo_dac)) + lower_bound->common.field32;
    uint32_t current_enc = (uint32_t)current;
    // always round up
    if (current > (float)current_enc)
    {
        current_enc++;
    }

    return current_enc;
}

// // complete interpolation function: given array and count of points and an ldo code, return the interpolated value
uint32_t power_vcpu_interpolate_from_points(const power_vcpu_interp_t* points, unsigned points_count, uint16_t ldo_dac)
{
    FPFW_RUNTIME_ASSERT(points != NULL);

    // find upper and lower bounds of fused points for the given ldo code
    const power_vcpu_interp_t* lower_bound = find_interpolation_bound(points, points_count, ldo_dac, true);
    const power_vcpu_interp_t* upper_bound = find_interpolation_bound(points, points_count, ldo_dac, false);
    if (lower_bound == upper_bound)
    {
        // attempt to find a lower point for gradient
        lower_bound = find_interpolation_bound(points, points_count, upper_bound->common.ldo_dac, true);
    }
    // use the upper and lower bound to interpolate a value for the given ldo_dac
    const uint32_t interpolated_value = power_vcpu_interpolate_value(ldo_dac, upper_bound, lower_bound);
    return interpolated_value;
}

void power_vcpu_precalculate_vf_currents(power_runconfig_t* p_runconfig, dvfs_config_t* p_dvfs_cfg)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);
    FPFW_RUNTIME_ASSERT(p_dvfs_cfg != NULL);

    // create a single adjustment for the types used within the dynamic current equation
    const float unit_adjustment_factor = FREQ_MHZ_TO_HZ * AF_TO_AF_PERCENT * C_DYN_PF_TO_F * VOLT_MV_TO_V;
    const unsigned vcpu_core_count = p_runconfig->fuses.tdp_config.num_cores;

    // iterate over the available VFTs
    for (unsigned vf_idx = 0; vf_idx < VFT_CURVESET_COUNT; ++vf_idx)
    {
        if (corebits_is_clear(&p_runconfig->derived.vfts[vf_idx].assigned_cores))
        {
            // no cores use this VFT
            continue;
        }
        // iterate over voltage/freq pairs in this table
        for (unsigned pstate_idx = p_runconfig->derived.vfts[vf_idx].min_plimit; pstate_idx < NUM_PSTATES; ++pstate_idx)
        {
            const uint16_t core_mv = p_runconfig->derived.vfts[vf_idx].vf[pstate_idx].voltage_mv;
            const uint16_t core_MHz = p_runconfig->derived.vfts[vf_idx].vf[pstate_idx].freq_Mhz;

            // the LDO code for this pstate
            /* TODO: several interpolations below require an ldo_dac code; determine which
             *       of the (up-to) 4 ITD-related curve's ldo_dac code we should be using.
             *       MIN/MAX value, etc.
             *
             *       https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1888554/
             */
            const uint16_t ldo_dac =
                p_runconfig->dvfs_vft.curveset[vf_idx].curve[vf_idx].vmat_info[0].ldo_dac_in[pstate_idx]; // p_dvfs_cfg->fuse_cfg.vft->vmat_info[0].ldo_dac_in[pstate_idx];

            // interpolate cdyn for this LDO code
            const uint32_t cdyn_pf = power_vcpu_interpolate_from_points(&p_runconfig->fuses.core_cdyn[0],
                                                                        DIMOF(p_runconfig->fuses.core_cdyn),
                                                                        ldo_dac);

            // calculate dynamic current V*F*Cdyn*AF - the interpolated Cdyn based on dhrystone
            const float core_dynamic_current = (float)core_mv * (float)core_MHz *
                                               (float)p_runconfig->knobs.activity_factor_dhry_adjustment *
                                               (float)cdyn_pf * unit_adjustment_factor;

            // interpolate ldo dynamic current for this LDO code
            const unsigned int vcpu_ldo_dyn_current =
                power_vcpu_interpolate_from_points(&p_runconfig->fuses.vcpu_ldo_dyn[0],
                                                   DIMOF(p_runconfig->fuses.vcpu_ldo_dyn),
                                                   ldo_dac);
            const float ldo_dynamic_current = ((float)vcpu_ldo_dyn_current / vcpu_core_count) * CURRENT_MA_TO_A;

            // interpolate reference leakage current for LDO code
            unsigned int vcpu_leakage_current =
                power_vcpu_interpolate_from_points(&p_runconfig->fuses.vcpu_leakage[0],
                                                   DIMOF(p_runconfig->fuses.vcpu_leakage),
                                                   ldo_dac);
            const float leakage_current = ((float)vcpu_leakage_current / vcpu_core_count) * CURRENT_MA_TO_A;

            // get a pointer to the precalculated entry for this vf & pstate
            power_vft_precalc_current_t* precalc = &p_runconfig->precalculated_current.curveset[vf_idx].vf[pstate_idx];

            // store the dynamic current as core + ldo_dynamic - these are not affected by temperature
            precalc->dynamic = core_dynamic_current + ldo_dynamic_current;

            // get the scaler associated with the temp the reference was taken at - likely result is 1
            const float leakage_scaler = power_vcpu_calc_core_leakage_scaler(
                p_runconfig,
                (p_runconfig->fuses.vcpu_leakage[0].current.temp_offset - FUSE_VCPU_TEMP_OFFSET));
            // store the leakage current as leakage / scaler - leakage current is affected by temperature
            precalc->leakage = leakage_current / leakage_scaler;

            // store values for diagnostic/CLI
            precalc->ref_leakage = leakage_current;
            precalc->dynamic_ldo = ldo_dynamic_current;
            precalc->cdyn_pf = cdyn_pf;

            POWER_LOG_INFO("Table %d  index %d  ldodac %d- total ldo dynamic %u, total leakage %u",
                           vf_idx,
                           pstate_idx,
                           ldo_dac,
                           vcpu_ldo_dyn_current,
                           vcpu_leakage_current);

            POWER_LOG_INFO("      per-core ldo dynamic %f, per-core leakage %f, dynamic %f",
                           ldo_dynamic_current,
                           leakage_current,
                           precalc->dynamic);
        }
    }
}