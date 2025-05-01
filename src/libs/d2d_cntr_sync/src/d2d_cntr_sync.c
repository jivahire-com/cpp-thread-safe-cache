//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file d2d_cntr_sync.c
 *  Sets up d2d sync for all subsystems for each die
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <bug_check.h>
#include <d2d_cntr_sync.h> // for d2d_counter_sync_init
#include <d2d_counter_knobs.h> // for d2d_counter_sync_configure_d0_and_enable_sync, d2d_counter_sync_configure_d1_params
#include <d2d_counter_sync.h> // for d2d_counter_sync_init
#include <fpfw_cfg_mgr.h>
#include <idhw.h>     // for idhw_is_single_die_boot_en
#include <idsw_kng.h> //for CPU_SCP
#include <stddef.h>   // for NULL
#include <stdint.h>   // for uint32_t

/*------------- Typedefs -----------------*/
typedef struct _d2d_cntr_sync_cache_t
{
    uint8_t die_num;
    uint8_t cpu_type;
    bool is_dual_die_boot;
    bool is_configured[2];
    bool is_enabled[2];
    d2d_counter_sync_d0_config_t d0_config;
    d2d_counter_sync_d1_config_t d1_config;
} d2d_cntr_sync_cache_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static d2d_cntr_sync_cache_t d2d_cntr_sync_data = {0};

/*------------- Functions ----------------*/
void d2d_cntr_sync_init(KNG_DIE_ID die_num, KNG_CPU_TYPE cpu_type)
{
    //! prevent re-initialization of d2d cntr sync data
    BUG_ASSERT(cpu_type == CPU_SCP);
    BUG_ASSERT(d2d_cntr_sync_data.is_enabled[die_num] == false);

    //! cache the die_num and cpu_type for this die
    d2d_cntr_sync_data.cpu_type = cpu_type;
    d2d_cntr_sync_data.die_num = die_num;
    d2d_cntr_sync_data.is_dual_die_boot = idhw_is_single_die_boot_en() ? false : true;

    //! d2d counter sync logic is only needed for dual die boot for scp cores
    if (d2d_cntr_sync_data.is_dual_die_boot)
    {
        //! the d2d counters are configured post the system counter init in gtimer_init()
        if (d2d_cntr_sync_data.die_num == DIE_0)
        {
            //! Get the d2d counter sync knobs, Each die has its own config params
            d2d_cntr_sync_data.d0_config = config_get_d2d_counter_sync_knobs().d2d_counter_sync_d0_cfg;
            //! Configure & Enable D2D counter sync logic for die 0
            d2d_counter_sync_configure_d0_and_enable_sync(&d2d_cntr_sync_data.d0_config);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Configured & Enabled for DIE 0\n");
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] D2D link/network delay between D0 and D1 :0x%lx\n",
                               d2d_cntr_sync_data.d0_config.network_delay);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Sync timeout (should be same for both dies) :0x%lx\n",
                               d2d_cntr_sync_data.d0_config.sync_timeout);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] D0 Update Addr Offset :0x%lx\n",
                               d2d_cntr_sync_data.d0_config.mstupdt_addr_offset);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Turn Around Time :0x%lx\n", d2d_cntr_sync_data.d0_config.turn_ar_time);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Calculated Round Trip Delay :0x%lx\n",
                               d2d_cntr_sync_data.d0_config.round_trip_delay);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Programmed Round Trip Delay :0x%lx\n",
                               d2d_cntr_sync_data.d0_config.use_prg_rndt_delay);
            d2d_cntr_sync_data.is_configured[DIE_0] = true;
            d2d_cntr_sync_data.is_enabled[DIE_0] = true;
        }
        else
        {
            d2d_cntr_sync_data.d1_config = config_get_d2d_counter_sync_knobs().d2d_counter_sync_d1_cfg;
            //! Only configure D2D counter sync logic for die 1
            //! This will be enabled post d2dss_sequence()
            d2d_counter_sync_configure_d1_params(&d2d_cntr_sync_data.d1_config);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Configured, Enable Pending for DIE 1\n");
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Counter Offset Threshold :0x%lx\n",
                               d2d_cntr_sync_data.d1_config.counter_offset_threshold);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Sync timeout (should be same for both dies) :0x%lx\n",
                               d2d_cntr_sync_data.d1_config.sync_timeout);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Sync Interval :0x%lx\n", d2d_cntr_sync_data.d1_config.sync_interval);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Turn Around Time :0x%lx\n", d2d_cntr_sync_data.d1_config.turn_ar_time);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Retry Count :0x%lx\n", d2d_cntr_sync_data.d1_config.retry_count);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Expected Update Time Threshold :0x%lx\n",
                               d2d_cntr_sync_data.d1_config.exp_updtime_threshold);
            d2d_cntr_sync_data.is_configured[DIE_1] = true;
        }
    }
}

void d2d_cntr_sync_enable(void)
{
    //! d2d sync counters are not supported on SVP currently
    if (!(IS_PLATFORM_SVP()))
    {
        //! Ensure die 1 is already configured and d2d link is up
        BUG_ASSERT(d2d_cntr_sync_data.is_dual_die_boot == true);
        BUG_ASSERT(d2d_cntr_sync_data.is_configured[DIE_1] == true);
        //! Enable d2d counter sync for SCP DIE 1
        if ((d2d_cntr_sync_data.die_num == DIE_1) && (d2d_cntr_sync_data.cpu_type == CPU_SCP))
        {
            d2d_counter_sync_enable(SOC_D1);
            FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Enabled for DIE 1\n");
        }
    }
}

void d2d_cntr_sync_reset(void)
{
    d2d_cntr_sync_data.die_num = 0;
    d2d_cntr_sync_data.cpu_type = 0;
    d2d_cntr_sync_data.is_dual_die_boot = false;
    d2d_cntr_sync_data.is_configured[DIE_0] = false;
    d2d_cntr_sync_data.is_configured[DIE_1] = false;
    d2d_cntr_sync_data.is_enabled[DIE_0] = false;
    d2d_cntr_sync_data.is_enabled[DIE_1] = false;
    memset(&d2d_cntr_sync_data.d0_config, 0, sizeof(d2d_counter_sync_d0_config_t));
    memset(&d2d_cntr_sync_data.d1_config, 0, sizeof(d2d_counter_sync_d1_config_t));
    FPFW_DBGPRINT_INFO("[D2D CNTR SYNC] Reset\n");
}