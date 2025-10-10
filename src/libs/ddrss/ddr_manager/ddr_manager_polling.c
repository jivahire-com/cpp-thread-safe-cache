//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_polling.c
 * This file contains the DIMM polling functionality.
 * DDR I3C calls through DFWK
 * BWL/Throttling
 * Telemetry reporting
 *
 *  BWL = Bandwidth Limiting - this is a throttling mechanism implemented in the DDR controller
 *
 * New for KNG: Each die will have 2 I3C controllers, addressing 3 DIMMs each
 */

/*------------- Includes -----------------*/
#include "ddr_manager_bwl.h"
#include "ddr_manager_i.h"
#include "ddr_manager_i3c.h"

#include <ErrorHandler.h> // for FPFwErrorRaise
#include <FPFwInterrupts.h>
#include <bug_check.h>
#include <cper.h>
#include <ddr_manager_events.h>
#include <ddrss.h>
#include <ddrss_intu.h>
#include <ddrss_lib.h>
#include <fpfw_cfg_mgr.h>
#include <health_monitor.h>
#include <interrupts.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void init_thresholds(ddr_dimm_temp_thresholds_t* thresholds);

/*-- Declarations (Statics and globals) --*/
ddr_manager_i3c_temperature_t ts0_temp;
ddr_manager_i3c_temperature_t ts1_temp;

/*------------- Functions ----------------*/
void ddr_read_dimm_temperatures()
{
    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        if (ddr_manager_temperature_sensor_read(dimm_idx, 0, &ts0_temp) == DDR_MANAGER_I3C_SUCCESS)
        {
            ddr_telemetry_update_dimm_temp(dimm_idx, 0, ts0_temp);
        }
        else
        {
            DDR_LOG_CRIT("Error reading DIMM %d TS0\n", dimm_idx);
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_READ_TEMPERATURE_SENSOR_0, dimm_idx);
        }

        if (ddr_manager_temperature_sensor_read(dimm_idx, 1, &ts1_temp) == DDR_MANAGER_I3C_SUCCESS)
        {
            ddr_telemetry_update_dimm_temp(dimm_idx, 1, ts1_temp);
        }
        else
        {
            DDR_LOG_CRIT("Error reading DIMM %d TS1\n", dimm_idx);
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_READ_TEMPERATURE_SENSOR_1, dimm_idx);
        }
    }
}

void ddr_read_dimm_power()
{
    uint16_t power_mW = 0;

    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        if (ddr_manager_power_mw_read(dimm_idx, &power_mW) == DDR_MANAGER_I3C_SUCCESS)
        {
            ddr_telemetry_update_dimm_power(dimm_idx, power_mW);
        }
        else
        {
            DDR_LOG_CRIT("Error reading DIMM %d power\n", dimm_idx);
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_PMIC_POWER_READ, dimm_idx);
        }
    }
}

// Checks cached DIMM temperatures against thresholds initially read from config knobs
// Thresholds may be overridden via CLI at runtime (ddr_bwl command) but will not affect config knobs values
// Above 'high' -> Engage BWL - unless bwl_force_disabled == true
// Below 'low'  -> If BWL engaged, disengage - unless bwl_force_enabled == true
// Above 'crit' -> Blow things up?  Task 2584104: Determine correct behavior when DDR exceeds critical temperature
void check_dimm_temp_thresholds()
{
    static bool is_first_run = true;
    static ddr_dimm_temp_thresholds_t thresholds = {0};

    acpi_err_sec_mem_vendor_t ddr_cper = {0};
    acpi_cper_section_t cper_section = {0};

    if (is_first_run)
    {
        init_thresholds(&thresholds);
        is_first_run = false;
    }

    uint16_t max_dimm_temp = 0;
    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        ddr_manager_i3c_temperature_t ts0_temp = ddr_telemetry_get_dimm_temp(dimm_idx, 0);
        ddr_manager_i3c_temperature_t ts1_temp = ddr_telemetry_get_dimm_temp(dimm_idx, 1);

        // Set the max_dimm_temp, accounting for negative temperatures
        if (ts0_temp.is_positive)
        {
            max_dimm_temp = ts0_temp.temp_int > max_dimm_temp ? ts0_temp.temp_int : max_dimm_temp;
        }

        if (ts1_temp.is_positive)
        {
            max_dimm_temp = ts1_temp.temp_int > max_dimm_temp ? ts1_temp.temp_int : max_dimm_temp;
        }

        if (!ts0_temp.is_positive && !ts1_temp.is_positive)
        {
            // If both temps are negative, set the max to 0
            max_dimm_temp = 0;
        }

        if ((ts0_temp.is_positive && ts0_temp.temp_int > thresholds.crit) ||
            (ts1_temp.is_positive && ts1_temp.temp_int > thresholds.crit))
        {
            DDR_LOG_CRIT("DIMM %d  %dC is above crit. threshold (%dC)\n", dimm_idx, max_dimm_temp, thresholds.crit);
            DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_DIMM_EXCEEDED_CRITICAL_TEMPERATURE_THRESHOLD, dimm_idx);

            memset(&ddr_cper, 0x0, sizeof(ddr_cper));
            prod_ddrss_get_intr_event_cper(dimm_idx * 2, DDRSS_INTU_MC_MEDIAREFTEMPCHANGED, &ddr_cper);

            memset(&cper_section, 0x0, sizeof(cper_section));
            cper_section.sec_ddr_mem_vendor = ddr_cper;
            hm_submit_cper(ACPI_ERROR_DOMAIN_DDR, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &cper_section, sizeof(acpi_err_sec_mem_vendor_t));

            // Blow things up
            ddr_manager_set_thermal_trip_gpio();
            FPFwErrorRaise(DDR_MANAGER_ET_TYPE_DIMM_EXCEEDED_CRITICAL_TEMPERATURE_THRESHOLD, dimm_idx, 0, 0, 0);
        }
    }

    // Engage BWL if the max DIMM temperature exceeds the high threshold
    // BWL = Bandwidth Limiting - this is a throttling mechanism implemented in the DDR controller
    if (max_dimm_temp > thresholds.high)
    {
        if (config_get_ddrmanager_bwl_en())
        {
            ddr_manager_enable_bwl_i3c();
        }
        else
        {
            DDR_LOG_WARN("BWL is disabled, but a DIMM temperature (%d) exceeded high threshold: %d\n",
                         max_dimm_temp,
                         thresholds.high);
            DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_DIMM_TEMPERATURES_EXCEED_HIGH_THRESHOLD_BWL_DISABLE);
        }
    }

    // Disengage BWL if the max DIMM temperature falls below the low threshold
    if (max_dimm_temp < thresholds.low)
    {
        ddr_manager_disable_bwl_i3c();
    }
}

void init_thresholds(ddr_dimm_temp_thresholds_t* thresholds)
{
    thresholds->low = config_get_ddrmanager_dimm_temp_low();
    thresholds->high = config_get_ddrmanager_dimm_temp_high();
    thresholds->crit = config_get_ddrmanager_dimm_temp_crit();
}

// Poll for reenabling ECC CE interrupts that were disabled in the RAS ISR handler
// after a CE error was detected.  This is to avoid interrupt storms if there are
// many CE errors occurring in a short period of time (HW bug workaround).
void ddr_poll_ecc_ce_errors()
{
    KNG_DIE_ID die_num = idsw_get_die_id();
    uint32_t ddrss_mask = ddrss_get_ddrss_mask();
    uint32_t base_mc = (die_num == DIE_1) ? DDRSS_MAX_MC_NUM_PER_DIE : 0;
    uint32_t irq_num;
    uint32_t mc;
    bool ddr_irq_en;
    bool ras_ce_irq_en;

    for (mc = base_mc; mc < base_mc + DDRSS_MAX_MC_NUM_PER_DIE; mc++)
    {
        if (!(ddrss_mask & (1 << DDRSS_MC_TO_DDRSS(mc))))
        {
            continue;
        }

        ras_ce_irq_en = false;
        prod_ddrss_get_ras_erg_ce_interrupt_enable(mc, DDRSS_RAS_NODE_ID_MC_ERG0, &ras_ce_irq_en);
        if (ras_ce_irq_en)
        {
            continue;
        }

        // The RAS CE interrupt is disabled currently,  re-enable it.
        DDR_LOG_INFO("Enabling RAS CE interrupt for MC%u\n", mc);
        irq_num = HW_INT_DDRSS0_COMBINED_SCP_INT + DDRSS_MC_TO_DDRSS(mc);
        ddr_irq_en = FPFwCoreInterruptIsEnabled(irq_num);
        if (ddr_irq_en)
        {
            // Diable DDR RAS interrupt to prevent race condition on
            // prod_ddrss_set_ras_erg_ce_interrupt_enable() in RAS ISR handler
            FPFwCoreInterruptDisableVector(irq_num);
        }
        // Re-enabling RAS CE interrupt
        prod_ddrss_set_ras_erg_ce_interrupt_enable(mc, DDRSS_RAS_NODE_ID_MC_ERG0, true);
        if (ddr_irq_en)
        {
            // Restore the original DDR RAS interrupt state
            FPFwCoreInterruptEnableVector(irq_num);
        }
    }
}
