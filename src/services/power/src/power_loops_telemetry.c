//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_loops_telemetry.c
 * Implements the power service telemetry loops
 */

/*------------- Includes -----------------*/
#include "pid_resource.h"
#include "power_events.h"
#include "power_hw_int_i.h"
#include "power_i.h"
#include "power_loops_i.h"
#include "power_runconfig_i.h"
#include "power_stub_i.h"

#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <corebits.h>
#include <debug.h>
#include <inttypes.h>
#include <pvt.h>        // for PVT_SUCCESS, reset_tile_pvt_dts_vm
#include <pvt_struct.h> // for pvt_alarm_setting_config_t, pvt_thres...
#include <scf_power.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
/*------------- Typedefs -----------------*/
typedef enum _pvt_type_t
{
    PVT_TYPE_VM,
    PVT_TYPE_DTS,
} pvt_type_t;

// Enum for telemetry types used in hw interface function
typedef enum _power_telem_type_t
{
    PM_TELEMETRY_VR_CURRENT = 0,
    PM_TELEMETRY_VR_TEMP,
    PM_TELEMETRY_SOC_TOP_TEMP,
    PM_TELEMETRY_SOC_TOP_VOLT,
} power_telem_type_t;

/*-------- Function Prototypes -----------*/
static void hw_send_telemetry(power_telem_type_t type);
static void hw_read_pvts();

// Following are function prototypes for state handlers (VR telemetry loop)
static void vr_telem_idle_handler(int event, const void* event_data);
static void vr_telem_current_telemetry_handler(int event, const void* event_data);
static void vr_telem_temp_telemetry_handler(int event, const void* event_data);
static void vr_telem_error_handler(int event, const void* event_data);

// Following are function prototypes for state handlers (PVT telemetry loop)
static void pvt_telem_idle_handler(int event, const void* event_data);
static void pvt_telem_pvt_telemetry_handler(int event, const void* event_data);
/*-- Declarations (Statics and globals) --*/

// Table of state handler functions for VR telemetry loop
static const power_state_handler_t vr_telem_loop_handler_table[POWER_VR_TELEM_STATE_MAX] = {
    [POWER_VR_TELEM_STATE_IDLE] = vr_telem_idle_handler,
    [POWER_VR_TELEM_STATE_CURRENT_TELEMETRY] = vr_telem_current_telemetry_handler,
    [POWER_VR_TELEM_STATE_TEMP_TELEMETRY] = vr_telem_temp_telemetry_handler,
    [POWER_VR_TELEM_STATE_ERROR] = vr_telem_error_handler,
};

// Table of state handler functions for PVT telemetry loop
static const power_state_handler_t pvt_telem_loop_handler_table[POWER_PVT_TELEM_STATE_MAX] = {
    [POWER_PVT_TELEM_STATE_IDLE] = pvt_telem_idle_handler,
    [POWER_PVT_TELEM_STATE_READ_PVT] = pvt_telem_pvt_telemetry_handler,
};

// residency data for telemetry loops
static power_loop_residency_t pvt_telemetry_loop_handler_residency[POWER_PVT_TELEM_STATE_MAX] = {0};
static power_loop_residency_t vr_telemetry_loop_handler_residency[POWER_VR_TELEM_STATE_MAX] = {0};

// loop context for telemetry loops
static power_loop_context_t s_pvt_telem_loop_context = {.state_count = POWER_PVT_TELEM_STATE_MAX,
                                                        .handlers = pvt_telem_loop_handler_table,
                                                        .residency = pvt_telemetry_loop_handler_residency,
                                                        .id = LOOP_ID_PVT_TELEM};

static power_loop_context_t s_vr_telem_loop_context = {.state_count = POWER_VR_TELEM_STATE_MAX,
                                                       .handlers = vr_telem_loop_handler_table,
                                                       .residency = vr_telemetry_loop_handler_residency,
                                                       .id = LOOP_ID_VR_TELEM};

static power_telem_loop_detail_t s_telem_loop = {0};

/*------------- Functions ----------------*/
static bool power_vr_telem_loop_retry_fail(power_loop_retries_t type)
{
    // call the common function with detail for vr telemetry loop
    return power_loops_retry_fail(&s_vr_telem_loop_context, type);
}

static void power_vr_telem_loop_change_state(power_vr_telem_state_t state)
{
    FPFW_RUNTIME_ASSERT(state < POWER_VR_TELEM_STATE_MAX);
    // call the common function with detail for vr telemetry loop
    return power_loops_change_state(&s_vr_telem_loop_context, (int)state);
}

void power_loops_vr_telem_handle_event(power_vr_telem_signal_t event, const void* event_data)
{
    // call the common function with detail for vr telemetry loop
    power_loops_handle_event(&s_vr_telem_loop_context, (int)event, event_data);
}

static void power_pvt_telem_loop_change_state(power_pvt_telem_state_t state)
{
    FPFW_RUNTIME_ASSERT(state < POWER_PVT_TELEM_STATE_MAX);
    // call the common function with detail for pvt telemetry loop
    return power_loops_change_state(&s_pvt_telem_loop_context, (int)state);
}

void power_loops_pvt_telem_handle_event(power_pvt_telem_signal_t event, const void* event_data)
{
    // call the common function with detail for pvt telemetry loop
    power_loops_handle_event(&s_pvt_telem_loop_context, (int)event, event_data);
}

/*-- VR telemetry loop state handlers --*/

static void vr_telem_idle_handler(int event, const void* event_data)
{
    UNUSED(event_data);

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY:
        // Returning to idle state
        POWER_LOG_TRACE("VR telemetry idle");
        break;
    case POWER_VR_TELEM_SIGNAL_INTERVAL:
        // Leaving idle state
        power_vr_telem_loop_change_state(POWER_VR_TELEM_STATE_CURRENT_TELEMETRY);
        break;
    default:
        break;
    }
}

static void vr_telem_current_telemetry_handler(int event, const void* event_data)
{
    UNUSED(event_data);

    // track the number of intervals for temp telemetry
    static uint16_t vr_telem_intervals = 0;
    // get the runtime configuration
    power_runconfig_t* p_runconfig = power_runconfig_get();

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY:
    case POWER_VR_TELEM_SIGNAL_VR_CURRENT_FAIL:
        // State entry or failure/retry condition
        // Request VR currents
        power_vrs_initiate_vr_reads();
        break;
    case POWER_VR_TELEM_SIGNAL_VR_CURRENT:
        if ((p_runconfig->knobs.loops_disable & power_loops_disable_t_VR_TELEM_LOOP) != 0)
        {
            // if VR telemetry loop is disabled, then we're done once successful collection of data is done
            power_vr_telem_loop_change_state(POWER_VR_TELEM_STATE_IDLE);
            return;
        }

        // store pointer to the VR data
        s_telem_loop.vr_data = (power_vrs_avs_latest_t*)event_data;

        // send_current_telemetry
        hw_send_telemetry(PM_TELEMETRY_VR_CURRENT);

        if (++vr_telem_intervals >= p_runconfig->knobs.temp_telemetry_divider)
        {
            vr_telem_intervals = 0;
            // proceed to temp telemetry
            power_vr_telem_loop_change_state(POWER_VR_TELEM_STATE_TEMP_TELEMETRY);
        }
        else
        {
            // nothing else to do, proceed to idle
            power_vr_telem_loop_change_state(POWER_VR_TELEM_STATE_IDLE);
        }

        break;
    case POWER_VR_TELEM_SIGNAL_INTERVAL:
        if (power_vr_telem_loop_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL))
        {
            power_vr_telem_loop_change_state(POWER_VR_TELEM_STATE_ERROR);
        }
        break;
    default:
        // ignore other unexpected
        break;
    }
}

static void vr_telem_temp_telemetry_handler(int event, const void* event_data)
{
    UNUSED(event);
    UNUSED(event_data);

    // currently requesting all AVS state in previous, this state
    // only for tracking time spent (passthrough)

    // send_temperature_telemetry
    hw_send_telemetry(PM_TELEMETRY_VR_TEMP);
    // proceed to idle
    power_vr_telem_loop_change_state(POWER_VR_TELEM_STATE_IDLE);
}

static void vr_telem_error_handler(int event, const void* event_data)
{
    UNUSED(event_data);

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY: // generic loop entry event
        POWER_ET_ERROR(POWER_ET_TYPE_VRTEL_ERROR_ENTRY,
                       POWER_ET_ENCODE_RETRIES_STATE(s_vr_telem_loop_context.status.retries[POWER_LOOP_RETRY_TYPE_INTERVAL],
                                                     s_vr_telem_loop_context.status.last_state));
        POWER_LOG_INFO(
            "VR telemetry loop error state entry, retries exhausted in state %d: interval retries %d",
            s_vr_telem_loop_context.status.last_state,
            s_vr_telem_loop_context.status.retries[POWER_LOOP_RETRY_TYPE_INTERVAL]);
        POWER_LOG_INFO("return to idle on next alarm");

        break;
    case POWER_VR_TELEM_SIGNAL_INTERVAL:
        // return to idle on next interval alarm.
        power_vr_telem_loop_change_state(POWER_VR_TELEM_STATE_IDLE);
        break;
    default:
        break;
    }
}

/*-- PVT telemetry loop state handlers --*/

static void pvt_telem_idle_handler(int event, const void* event_data)
{
    UNUSED(event_data);

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY:
        // Returning to idle state
        POWER_LOG_TRACE("PVT telemetry idle");
        break;
    case POWER_PVT_TELEM_SIGNAL_INTERVAL:
        // Leaving idle state
        power_pvt_telem_loop_change_state(POWER_PVT_TELEM_STATE_READ_PVT);
        break;
    default:
        break;
    }
}

static void pvt_telem_pvt_telemetry_handler(int event, const void* event_data)
{
    UNUSED(event);
    UNUSED(event_data);

    // call interface function to collect pvt data
    hw_read_pvts();

    // call interface function to send soc top telemetry
    hw_send_telemetry(PM_TELEMETRY_SOC_TOP_TEMP);
    hw_send_telemetry(PM_TELEMETRY_SOC_TOP_VOLT);

    power_pvt_telem_loop_change_state(POWER_PVT_TELEM_STATE_IDLE);
}

void power_loops_telemetry_init()
{
    // initialize the loop context
    power_loops_init_loop(&s_vr_telem_loop_context);
    power_loops_init_loop(&s_pvt_telem_loop_context);
}

/* function to update one DTS or VM channel's sample data */
static void update_pvt_data(pvt_type_t type, unsigned index, uint16_t sample, uint16_t sample_raw)
{
    power_telemetry_pvt_state_t* state_data = NULL;
    unsigned index_count = 0;

    switch (type)
    {
    case PVT_TYPE_VM:
        state_data = s_telem_loop.soc_top_voltage_data;
        index_count = SOC_PVT_TOTAL_CHANNELS_VM;
        break;
    case PVT_TYPE_DTS:
        state_data = s_telem_loop.soc_top_temp_data;
        index_count = SOC_PVT_TOTAL_CHANNELS_DTS;
        break;
    }
    // catch this code issue if it exists
    ASSERT(index < index_count);

    state_data = &state_data[index];
    ++state_data->valid_samples;
    if (sample > state_data->sample_hi)
    {
        state_data->sample_hi = sample;
    }
    if ((sample < state_data->sample_lo) || (state_data->valid_samples == 1))
    {
        state_data->sample_lo = sample;
    }
    state_data->last_sample = sample;
    state_data->last_sample_raw = sample_raw;
}

/* for each dts sample, if valid, convert to temperature, and update our sample
 * data */
static void process_pvt_dts_samples(pvt_irq_soc_dts_data_t* samples, const power_fuse_data_t* fuses)
{
    uint16_t max_temp = 0; // to track max soc temp
    for (unsigned i = 0; i < SOC_PVT_TOTAL_CHANNELS_DTS; ++i)
    {
        if (samples->valid_bits & (1 << i))
        {
            // for now storing in 0.1C increments, consistent with AVS
            uint16_t converted = power_hw_dts_pvt_raw_to_temp_dC(samples->sample_data[i], fuses->dts_coeff_soctop[i]);
            update_pvt_data(PVT_TYPE_DTS, i, converted, samples->sample_data[i]);
            // store the max converted temperature
            if (converted > max_temp)
            {
                max_temp = converted;
            }
        }
        else
        {
            // if temp samples weren't valid, max out max_temp for this iteration
            max_temp = UINT16_MAX;
        }
    }
    // store the maximum encountered soc temperature
    s_telem_loop.soc_max_temp_dC = max_temp;
}

/* for each vm sample, if valid, convert to temperature, and update our sample
 * data */
static void process_pvt_vm_samples(pvt_irq_soc_vm_data_t* samples, const power_service_config_t* p_sconfig)
{
    for (unsigned i = 0; i < SOC_PVT_TOTAL_CHANNELS_VM; ++i)
    {
        if (samples->valid_bits & (1 << i))
        {
            // for now storing as mV as this matches AVS
            const bool div_by_2 = ((p_sconfig->soc_vm[i].flags & VM_FLAGS_DIV2) != 0);
            uint16_t converted =
                (uint16_t)FLOAT_TO_UNSIGNED((DOUT2VOLTS(samples->sample_data[i])) * (div_by_2 ? 2000 : 1000));
            update_pvt_data(PVT_TYPE_VM, i, converted, samples->sample_data[i]);
        }
    }
}

static void hw_read_pvts()
{
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_sconfig = p_runconfig->p_sconfig;
    pvt_irq_soc_vm_data_t vm_samples = {0};
    pvt_irq_soc_dts_data_t dts_samples = {0};

    // do not want to clear alarmb as this feeds thermtrip; soc hot is latched in force_pmin (io_temp bit)
    int status = soc_pvt_handle_dts_irq_exclear(p_sconfig->soc_pvt_base, &dts_samples, PVT_IRQ_ALARMA | PVT_IRQ_DONE);
    // this will only fail if we pass in bad parameters, so assert to catch sw bugs
    ASSERT(status == PVT_SUCCESS);

    // do not want to clear either alarm interrupt as these feed pd_fault
    status = soc_pvt_handle_vm_irq_exclear(p_sconfig->soc_pvt_base, &vm_samples, PVT_IRQ_DONE);
    // this will only fail if we pass in bad parameters, so assert to catch sw bugs
    ASSERT(status == PVT_SUCCESS);

    process_pvt_dts_samples(&dts_samples, &p_runconfig->fuses);
    process_pvt_vm_samples(&vm_samples, p_sconfig);

    // handle clear of force pmin if necessary
    power_hw_check_io_temp_force_pmin(s_telem_loop.soc_max_temp_dC);
}

static void hw_send_telemetry(power_telem_type_t type)
{
    FPFW_UNUSED(type);

    /* TODO:  https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491034/
    // slightly updated pioneer implementation below

    #define QUADSIZE(x) ((sizeof(x) + 7) / 8)  // quadword size

    sensor_ram_entry_t entry = {0};
    uint32_t fifo_id         = MAX_FIFO_ID;
    uint32_t entry_qsize;

    // all entries require a timestamp
    entry.common.time_stamp = power_timer_get_counter();
    switch (type) {
        case PM_TELEMETRY_SOC_TOP_TEMP:
            // ensure there's enough available storage in the provided type
            ASSERT(SOC_PVT_TOTAL_CHANNELS_DTS <= DIMOF(entry.soc_pvt_temp.sensor_temp));
            for (int pvt_idx = 0; pvt_idx < SOC_PVT_TOTAL_CHANNELS_DTS; ++pvt_idx) {
                entry.soc_pvt_temp.sensor_temp[pvt_idx] = s_telem_loop.soc_top_temp_data[pvt_idx].last_sample;
            }
            fifo_id     = SOC_PVT_TEMP_FIFO;
            entry_qsize = QUADSIZE(entry.soc_pvt_temp);
            break;
        case PM_TELEMETRY_SOC_TOP_VOLT:
            // ensure there's enough available storage in the provided type
            ASSERT(SOC_PVT_TOTAL_CHANNELS_VM <= DIMOF(entry.soc_pvt_voltage.sensor_voltage));
            for (int pvt_idx = 0; pvt_idx < SOC_PVT_TOTAL_CHANNELS_VM; ++pvt_idx) {
                entry.soc_pvt_voltage.sensor_voltage[pvt_idx] = s_telem_loop.soc_top_voltage_data[pvt_idx].last_sample;
            }
            fifo_id     = SOC_PVT_VOLTAGE_FIFO;
            entry_qsize = QUADSIZE(entry.soc_pvt_voltage);
            break;
        case PM_TELEMETRY_VR_TEMP:
            // always expect that we have pointer to data if we're here
            FPFW_RUNTIME_ASSERT(s_telem_loop.p_vr_data != NULL);
            // ensure there's enough available storage in the provided type
            ASSERT(MPCL_VR_COUNT <= DIMOF(entry.vr_temp.vr_temp));
            for (int vr_idx = 0; vr_idx < MPCL_VR_COUNT; ++vr_idx) {
                entry.vr_temp.vr_temp[vr_idx] = s_telem_loop.vr_inputs[vr_idx].temperature;
            }
            fifo_id     = VR_TEMP_BUFFER_FIFO;
            entry_qsize = QUADSIZE(entry.vr_temp);
            break;
        case PM_TELEMETRY_VR_CURRENT:
            // always expect that we have pointer to data if we're here
            FPFW_RUNTIME_ASSERT(s_telem_loop.p_vr_data != NULL);
            // ensure there's enough available storage in the provided type
            ASSERT(MPCL_VR_COUNT <= DIMOF(entry.vr_current.vr_current));
            ASSERT(MPCL_VR_COUNT <= DIMOF(entry.vr_current.vr_voltage));
            for (int vr_idx = 0; vr_idx < MPCL_VR_COUNT; ++vr_idx) {
                entry.vr_current.vr_current[vr_idx] = s_telem_loop.vr_inputs[vr_idx].current;
                entry.vr_current.vr_voltage[vr_idx] = s_telem_loop.vr_inputs[vr_idx].voltage;
            }
            entry_qsize = QUADSIZE(entry.vr_current);
            fifo_id     = VR_CURRENT_BUFFER_FIFO;
            break;
    }
    ASSERT(fifo_id != MAX_FIFO_ID);
    if (fifo_id != MAX_FIFO_ID) {
        int status;
        status = sensor_fifo_api->mod_sensor_fifo_add_entry(fifo, data, size);
        ASSERT(status == FWK_SUCCESS);
    }
    */
}
