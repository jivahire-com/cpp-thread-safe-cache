//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_report_flush.c
 * Implements GHES flush related functionality.
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <crash_dump.h>
#include <fpfw_pldm_service.h>
#include <fpfw_status.h>
#include <health_monitor_i.h>
#include <icc_platform_defines.h>
#include <tx_timer.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static TX_TIMER hm_flush_timer = {0};
static const uint8_t ghes_flush_timeout_in_secs = 3;

/*------------- Functions ----------------*/

static void hm_flush_ghes_callback(ULONG input)
{
    FPFW_UNUSED(input);

    bool report_needed = false;
    volatile uint64_t* err_record_addr = (uint64_t*)get_hm_config()->mscp_ghes_error_record_addr_table_base;

    for (uint32_t error_domain_idx = 0; error_domain_idx < ACPI_ERROR_DOMAIN_COUNT; error_domain_idx++)
    {
        volatile acpi_ghes_error_record_dual_die_t* current_error_status_block =
            (volatile acpi_ghes_error_record_dual_die_t*)MSCP_GHES_ADDR(*err_record_addr);

        if (current_error_status_block->block_status_entry_count != 0)
        {
            report_needed = true;
            break;
        }
        err_record_addr++;
    }

    if (report_needed)
    {
        hm_report_error_event(HM_ERROR_REPORT_INTERRUPT, true);
    }
    else
    {
        UINT ret = tx_timer_deactivate(&hm_flush_timer);
        BUG_ASSERT(ret == TX_SUCCESS);
    }
}

void hm_flush_GHES_until_OS_boot()
{
    if (hm_allow_ras_reporting() == false)
    {
        return;
    }

    if (hm_flush_timer.tx_timer_id == TX_TIMER_ID)
    {
        return;
    }

    UINT ret = tx_timer_create(&hm_flush_timer,
                               "hm_flush",
                               hm_flush_ghes_callback,
                               0,
                               ghes_flush_timeout_in_secs * TX_TIMER_TICKS_PER_SECOND,
                               ghes_flush_timeout_in_secs * TX_TIMER_TICKS_PER_SECOND,
                               TX_AUTO_ACTIVATE);
    BUG_ASSERT(ret == TX_SUCCESS);
}