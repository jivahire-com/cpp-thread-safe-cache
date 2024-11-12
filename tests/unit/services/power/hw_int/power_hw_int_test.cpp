//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_hw_int_test.cpp
 * Power service hw interface tests (dvfs, odcm, pvt, etc)
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t

extern "C" {
#include "mock_dvfs.h" // for mock_dvfs_last_config, mock_dvfs_...
#include "mock_odcm.h" // for mock_odcm_last_config
#include "mock_pvt.h"  // for mock_pvt_last_soc_config, mock_pv...

#include <CMockaWrapper.h>         // for CmockaWrapperTest, assert_int_equal
#include <core_cluster_top_regs.h> // for CORE_CLUSTER_TOP_CORE_CLUSTER0_AD...
#include <corebits.h>              // for corebits_set_bit
#include <dvfs.h>                  // for dvfs_get_cppc_from_pstate, _DVFS_...
#include <dvfs_struct.h>           // for DVFS_DEF_PLIMIT_INDEX_NOMINAL
#include <fgpll_regs.h>            // for (anonymous), fgpll_reg, ptr_fgpll...
#include <mock_bug_check.h>        // for __wrap_crash_dump_bug_check
#include <odcm.h>                  // for _ODCM_RETCODE
#include <odcm_struct.h>           // for ODCM_DEFAULT_CONFIG, odcm_config_t
#include <pex_regs.h>              // for PEX_CORE_PLL_ADDRESS
#include <power_hw_int_i.h>        // for power_init_core, power_init_soc
#include <power_i.h>               // for TEMP2DOUT_FUSED
#include <power_runconfig.h>       // for power_knobs_t, power_fuse_data_t
#include <power_runconfig_i.h>     // for power_runconfig_t
#include <pvt.h>                   // for _PVT_RETCODE
#include <pvt_struct.h>            // for VOLTS2DOUT, pvt_alarm_setting_con...
#include <scp_exp_csr_regs.h>
#include <silibs_common.h> // for ARRAY_SIZE, MAX
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <string.h> // for memset

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

#define TEST_CORE_COUNT 1 // hard-coded number of cores in power FW for now
#define MAX_10BIT       1023
#define MAX_16BIT       0xFFFF
#define CLUSTER_STRIDE  (CORE_CLUSTER_TOP_CORE_CLUSTER1_ADDRESS - CORE_CLUSTER_TOP_CORE_CLUSTER0_ADDRESS)

#define CLIP_PVT(dout) (uint16_t)(dout > MAX_16BIT ? MAX_16BIT : dout)
// temperature thresholds are in 0.1C increments, divide by 10 to get value for dout
#define TEMPTHRESHOLD2DOUT(threshold, kval, yval) \
    CLIP_PVT(TEMP2DOUT_FUSED(((float)threshold / 10.0f), kval, yval))
// voltage thresholds are in mV increments, divide by 1000 to get value for dout
#define VOLTTHRESHOLD2DOUT(threshold) CLIP_PVT(VOLTS2DOUT((float)threshold / 1000.0f))

static uint8_t cluster_mem[CLUSTER_STRIDE * TEST_CORE_COUNT];
static power_telcfg_t s_telcfg = {0};
static _power_service_config_t s_config = {
    .cluster_pex_base = (uintptr_t)cluster_mem,
    .cluster_stride = CLUSTER_STRIDE,
};
static power_runconfig_t s_runconfig = {.p_sconfig = &s_config};

extern avs_pwr_request_context_t pwr_avs_request[MAX_AVS_INST];

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_dvfs_ns_set_cppc_desired2(const uintptr_t cluster_pex_base_addr,
                                      uint8_t cppc_desired,
                                      uint8_t cppc_base_perf,
                                      uint8_t throttle_pri,
                                      uint8_t boost_pri)
{
    check_expected(cluster_pex_base_addr);
    check_expected(cppc_desired);
    check_expected(cppc_base_perf);
    check_expected(throttle_pri);
    check_expected(boost_pri);
    return;
}

void __wrap_scp_avs_client_write(PDFWK_INTERFACE_HEADER Interface,
                                 PDFWK_ASYNC_REQUEST_HEADER Request,
                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                 void* CompletionContext)
{
    check_expected(Interface);
    check_expected(Request);
    check_expected(CompletionRoutine);
    check_expected(CompletionContext);
    function_called();
    return;
}

bool __wrap_all_requests_completed(avs_pwr_request_context_t* pwr_avs_request, uint8_t avs_bus)
{
    UNUSED(pwr_avs_request);
    UNUSED(avs_bus);
    function_called();
    return true;
}

void __wrap_power_loops_control_handle_event(power_ctrl_loop_signal_t event, const void* event_data)
{
    UNUSED(event);
    UNUSED(event_data);
    return;
}

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/

// wrap for power_runconfig_get
power_runconfig_t* __wrap_power_runconfig_get()
{
    return mock_type(power_runconfig_t*);
}

} // extern "C"

void base_telemetry()
{
    memset(&s_telcfg, 0, sizeof(s_telcfg));
}

void base_config()
{
    s_runconfig.derived.pnominal = DVFS_DEF_PLIMIT_INDEX_NOMINAL;
    const dvfs_vft_t default_vft = DVFS_VFT_DEFAULT_CONFIG;
    s_runconfig.dvfs_vft.curveset[0].curve[0] = default_vft;

    power_fuse_data_t* f = &s_runconfig.fuses;

    for (unsigned idx = 0; idx < ARRAY_SIZE(f->dts_coeff_tile); ++idx)
    {
        f->dts_coeff_tile[idx].k_val = 297; // these are default values pulled from PVT spec
        f->dts_coeff_tile[idx].y_val = 703; //  tests do not need them to be anything specific
    }

    for (unsigned idx = 0; idx < ARRAY_SIZE(f->dts_coeff_soctop); ++idx)
    {
        f->dts_coeff_soctop[idx].k_val = 300; // values for tile/soctop will be the same initially, but make them
        f->dts_coeff_soctop[idx].y_val =
            640; //  different here to ensure code is using the right coefficients for specific calculations
    }

    power_knobs_t* knobs = &s_runconfig.knobs;

    knobs->mpmm.enable = true;
    knobs->mpmm.gear = 2; // Throttle power viruses only. - POR

    knobs->current_threshold.iref_to_max_percent = 80;
    knobs->current_threshold.t1_percent = 90;
    knobs->current_threshold.t2_percent = 95;
    knobs->current_threshold.t3_percent = 100;
    knobs->current_throt.telemetry_epoch_count = 10; // 10-20 valid
    knobs->current_throt.rolling_window_count = 5;   // max 7
    knobs->current_throt.inc_ctr = 0x1f;             // 0x0-0x1f valid
    knobs->current_throt.dec_ctr = 0x1f;             // 0x0-0x1f valid
    knobs->current_throt.inc_amt = 0;                // 0-3 == 1-4
    knobs->current_throt.dec_amt0 = 1;               // 0-3 == 1-4
    knobs->current_throt.dec_amt1 = 2;               // 0-3 == 1-4

    knobs->adclk_throt.enable = false; // start disabled for test
    knobs->adclk_throt.inc_ctr = 0x1f; // 0x0-0x1f valid
    knobs->adclk_throt.dec_ctr = 0x1f; // 0x0-0x1f valid
    knobs->adclk_throt.inc_amt = 0;    // 0-3 == 1-4
    knobs->adclk_throt.dec_amt = 1;    // 0-3 == 1-4

    knobs->tile_temp_throt.thermtrip.hyst_threshold = 850;
    knobs->tile_temp_throt.thermtrip.alarm_threshold = 1050;
    knobs->tile_temp_throt.hot.hyst_threshold = 900;
    knobs->tile_temp_throt.hot.alarm_threshold = 950;
    knobs->tile_temp_throt.inc_ctr = 0xf; // 0x0-0xf valid
    knobs->tile_temp_throt.dec_ctr = 0xf; // 0x0-0xf valid
    knobs->tile_temp_throt.inc_amt = 0;   // 0-3 == 1-4
    knobs->tile_temp_throt.dec_amt = 1;   // 0-3 == 1-4

    for (int idx = 0; idx < TILE_PVT_NUM_CHANNELS_VM; ++idx)
    {
        knobs->tile_vm.thresholds[idx].overvolt.hyst_threshold = 600; // just need a somewhat valid default mV value
        knobs->tile_vm.thresholds[idx].overvolt.alarm_threshold = 800; // for overvolt, hyst->alarm should be rising
        knobs->tile_vm.thresholds[idx].undervolt.hyst_threshold = 800; // for undervolt, hyst->alarm should be dropping
        knobs->tile_vm.thresholds[idx].undervolt.alarm_threshold = 600;
    }

    knobs->soc_temp.thermtrip.hyst_threshold = 850;
    knobs->soc_temp.thermtrip.alarm_threshold = 1000;
    knobs->soc_temp.hot.hyst_threshold = 850;
    knobs->soc_temp.hot.alarm_threshold = 900;

    for (int idx = 0; idx < SOC_PVT_TOTAL_CHANNELS_VM; ++idx)
    {
        knobs->soc_vm.thresholds[idx].overvolt.hyst_threshold = 600; // just need a somewhat valid default mV value
        knobs->soc_vm.thresholds[idx].overvolt.alarm_threshold = 800; // for overvolt, hyst->alarm should be rising
        knobs->soc_vm.thresholds[idx].undervolt.hyst_threshold = 800; // for undervolt, hyst->alarm should be dropping
        knobs->soc_vm.thresholds[idx].undervolt.alarm_threshold = 600;
    }
    // expect that pll lock error is enabled
    knobs->plllock_cfg.enable_error = true;

    // core vft config
    for (unsigned int core_idx = 0; core_idx < TEST_CORE_COUNT; ++core_idx)
    {
        s_runconfig.derived.assigned_vft[core_idx] = core_idx % VFT_CURVESET_COUNT;
        s_runconfig.derived.vfts[core_idx % VFT_CURVESET_COUNT].min_plimit = 0;
        // ensure cores are valid
        corebits_set_bit(&s_runconfig.fuses.valid_cores, core_idx);
    }

    knobs->force_pstate = NUM_PSTATES;

    // default enable all PVT alarms
    knobs->soc_vm_overvolt_en = true;
    knobs->soc_vm_undervolt_en = true;
    knobs->tile_vm_overvolt_en = true;
    knobs->tile_vm_undervolt_en = true;
    knobs->soc_temp_hot_en = true;
    knobs->soc_temp_thermtrip_en = true;
    knobs->tile_temp_hot_en = true;
    knobs->tile_temp_thermtrip_en = true;
}

// converts knob settings to expected config values in our global config
void validate_odcm_cfg(unsigned int core, bool telem_only)
{
    odcm_config_t local_odcm_cfg = ODCM_DEFAULT_CONFIG;
    const power_knobs_t* knobs = &s_runconfig.knobs;

    // propagate knobs to config the way we expect FW to do so
    local_odcm_cfg.do_fuse_config = false;
    local_odcm_cfg.mma_cfg.rolling_window_count = knobs->current_throt.rolling_window_count;
    local_odcm_cfg.mma_cfg.epoch_count = knobs->current_throt.telemetry_epoch_count;
    local_odcm_cfg.mma_cfg.resolution_mode = 0;
    local_odcm_cfg.telem_cfg.buffer_start_addr =
        s_telcfg.current_telem_start_addr + (core * s_telcfg.current_telem_entry_size);
    local_odcm_cfg.telem_cfg.buffer_size = s_telcfg.current_telem_buffer_size;
    local_odcm_cfg.telem_cfg.buffer_stride = s_telcfg.current_telem_stride_size;

    if (telem_only)
    {
        assert_memory_equal(&mock_odcm_last_config()->telem_cfg,
                            &local_odcm_cfg.telem_cfg,
                            sizeof(local_odcm_cfg.telem_cfg));
    }
    else
    {
        assert_memory_equal(mock_odcm_last_config(), &local_odcm_cfg, sizeof(local_odcm_cfg));
    }
}

void validate_pvt(const pvt_channels_setting_config_t* pvt_channel_cfg, pvt_alarm_setting_config_t* alarm_setting, unsigned count)
{
    assert_int_equal(pvt_channel_cfg->setting_count, count);
    for (unsigned int idx = 0; idx < count; ++idx)
    {
        assert_memory_equal(alarm_setting, &pvt_channel_cfg->alarm_settings[idx], sizeof(pvt_alarm_setting_config_t));
    }
}

void validate_vm_pvt(const pvt_channels_setting_config_t* pvt_channel_cfg,
                     pvt_alarm_setting_config_t* alarm_setting,
                     pvt_alarm_setting_config_t* alarm_div2_setting,
                     const power_vm_detail_t* detail,
                     unsigned count)
{
    assert_int_equal(pvt_channel_cfg->setting_count, count);
    for (unsigned int idx = 0; idx < count; ++idx)
    {
        if (detail[idx].flags & VM_FLAGS_DIV2)
        {
            assert_memory_equal(alarm_div2_setting, &pvt_channel_cfg->alarm_settings[idx], sizeof(pvt_alarm_setting_config_t));
        }
        else
        {
            assert_memory_equal(alarm_setting, &pvt_channel_cfg->alarm_settings[idx], sizeof(pvt_alarm_setting_config_t));
        }
    }
}

void validate_tile_pvt_dts(unsigned int tile)
{
    const power_knobs_t* knobs = &s_runconfig.knobs;
    const power_fuse_data_t* fuses = &s_runconfig.fuses;
    const pvt_setting_config_t* pvt_config = mock_pvt_last_tile_config();

    pvt_alarm_setting_config_t dts_alarm_setting = {0};
    dts_alarm_setting.alarma_enable = true;
    dts_alarm_setting.alarmb_enable = true;
    dts_alarm_setting.alarma_thresholds.hyst_threshold = TEMPTHRESHOLD2DOUT(knobs->tile_temp_throt.hot.hyst_threshold,
                                                                            fuses->dts_coeff_tile[tile].k_val,
                                                                            fuses->dts_coeff_tile[tile].y_val);
    dts_alarm_setting.alarma_thresholds.alarm_threshold = TEMPTHRESHOLD2DOUT(knobs->tile_temp_throt.hot.alarm_threshold,
                                                                             fuses->dts_coeff_tile[tile].k_val,
                                                                             fuses->dts_coeff_tile[tile].y_val);
    dts_alarm_setting.alarmb_thresholds.hyst_threshold = TEMPTHRESHOLD2DOUT(knobs->tile_temp_throt.thermtrip.hyst_threshold,
                                                                            fuses->dts_coeff_tile[tile].k_val,
                                                                            fuses->dts_coeff_tile[tile].y_val);
    dts_alarm_setting.alarmb_thresholds.alarm_threshold = TEMPTHRESHOLD2DOUT(knobs->tile_temp_throt.thermtrip.alarm_threshold,
                                                                             fuses->dts_coeff_tile[tile].k_val,
                                                                             fuses->dts_coeff_tile[tile].y_val);

    validate_pvt(&pvt_config->dts_channels_config, &dts_alarm_setting, TILE_PVT_NUM_CHANNELS_DTS);
}

void validate_tile_pvt_vm()
{
    const power_knobs_t* knobs = &s_runconfig.knobs;
    const pvt_setting_config_t* pvt_config = mock_pvt_last_tile_config();
    const power_service_config_t* config = s_runconfig.p_sconfig;

    pvt_alarm_setting_config_t vm_alarm_setting = {0};
    vm_alarm_setting.alarma_enable = true;
    vm_alarm_setting.alarmb_enable = true;
    vm_alarm_setting.alarma_thresholds.hyst_threshold =
        VOLTTHRESHOLD2DOUT(knobs->tile_vm.thresholds[0].overvolt.hyst_threshold);
    vm_alarm_setting.alarma_thresholds.alarm_threshold =
        VOLTTHRESHOLD2DOUT(knobs->tile_vm.thresholds[0].overvolt.alarm_threshold);
    vm_alarm_setting.alarmb_thresholds.hyst_threshold =
        VOLTTHRESHOLD2DOUT(knobs->tile_vm.thresholds[0].undervolt.hyst_threshold);
    vm_alarm_setting.alarmb_thresholds.alarm_threshold =
        VOLTTHRESHOLD2DOUT(knobs->tile_vm.thresholds[0].undervolt.alarm_threshold);

    pvt_alarm_setting_config_t vm_alarm_setting2 = {0};
    vm_alarm_setting2.alarma_enable = true;
    vm_alarm_setting2.alarmb_enable = true;
    vm_alarm_setting2.alarma_thresholds.hyst_threshold =
        VOLTTHRESHOLD2DOUT(((float)knobs->tile_vm.thresholds[0].overvolt.hyst_threshold) / 2.0f);
    vm_alarm_setting2.alarma_thresholds.alarm_threshold =
        VOLTTHRESHOLD2DOUT(((float)knobs->tile_vm.thresholds[0].overvolt.alarm_threshold) / 2.0f);
    vm_alarm_setting2.alarmb_thresholds.hyst_threshold =
        VOLTTHRESHOLD2DOUT(((float)knobs->tile_vm.thresholds[0].undervolt.hyst_threshold) / 2.0f);
    vm_alarm_setting2.alarmb_thresholds.alarm_threshold =
        VOLTTHRESHOLD2DOUT(((float)knobs->tile_vm.thresholds[0].undervolt.alarm_threshold) / 2.0f);

    validate_vm_pvt(&pvt_config->vm_channels_config, &vm_alarm_setting, &vm_alarm_setting2, config->tile_vm, TILE_PVT_NUM_CHANNELS_VM);
}

void validate_soc_pvt_dts()
{
    const power_knobs_t* knobs = &s_runconfig.knobs;
    const power_fuse_data_t* fuses = &s_runconfig.fuses;
    const pvt_setting_config_t* pvt_config = mock_pvt_last_soc_config();

    pvt_alarm_setting_config_t dts_alarm_setting = {0};
    dts_alarm_setting.alarma_enable = true;
    dts_alarm_setting.alarmb_enable = true;
    dts_alarm_setting.alarma_thresholds.hyst_threshold = TEMPTHRESHOLD2DOUT(knobs->soc_temp.hot.hyst_threshold,
                                                                            fuses->dts_coeff_soctop[0].k_val,
                                                                            fuses->dts_coeff_soctop[0].y_val);
    dts_alarm_setting.alarma_thresholds.alarm_threshold = TEMPTHRESHOLD2DOUT(knobs->soc_temp.hot.alarm_threshold,
                                                                             fuses->dts_coeff_soctop[0].k_val,
                                                                             fuses->dts_coeff_soctop[0].y_val);
    dts_alarm_setting.alarmb_thresholds.hyst_threshold = TEMPTHRESHOLD2DOUT(knobs->soc_temp.thermtrip.hyst_threshold,
                                                                            fuses->dts_coeff_soctop[0].k_val,
                                                                            fuses->dts_coeff_soctop[0].y_val);
    dts_alarm_setting.alarmb_thresholds.alarm_threshold = TEMPTHRESHOLD2DOUT(knobs->soc_temp.thermtrip.alarm_threshold,
                                                                             fuses->dts_coeff_soctop[0].k_val,
                                                                             fuses->dts_coeff_soctop[0].y_val);

    validate_pvt(&pvt_config->dts_channels_config, &dts_alarm_setting, SOC_PVT_TOTAL_CHANNELS_DTS);
}

void validate_soc_pvt_vm()
{
    const power_knobs_t* knobs = &s_runconfig.knobs;
    const pvt_setting_config_t* pvt_config = mock_pvt_last_soc_config();
    const power_service_config_t* config = s_runconfig.p_sconfig;

    pvt_alarm_setting_config_t vm_alarm_setting = {0};
    vm_alarm_setting.alarma_enable = true;
    vm_alarm_setting.alarmb_enable = true;
    vm_alarm_setting.alarma_thresholds.hyst_threshold =
        VOLTTHRESHOLD2DOUT(knobs->soc_vm.thresholds[0].overvolt.hyst_threshold);
    vm_alarm_setting.alarma_thresholds.alarm_threshold =
        VOLTTHRESHOLD2DOUT(knobs->soc_vm.thresholds[0].overvolt.alarm_threshold);
    vm_alarm_setting.alarmb_thresholds.hyst_threshold =
        VOLTTHRESHOLD2DOUT(knobs->soc_vm.thresholds[0].undervolt.hyst_threshold);
    vm_alarm_setting.alarmb_thresholds.alarm_threshold =
        VOLTTHRESHOLD2DOUT(knobs->soc_vm.thresholds[0].undervolt.alarm_threshold);

    pvt_alarm_setting_config_t vm_alarm_setting2 = {0};
    vm_alarm_setting2.alarma_enable = true;
    vm_alarm_setting2.alarmb_enable = true;
    vm_alarm_setting2.alarma_thresholds.hyst_threshold =
        VOLTTHRESHOLD2DOUT(((float)knobs->soc_vm.thresholds[0].overvolt.hyst_threshold) / 2.0f);
    vm_alarm_setting2.alarma_thresholds.alarm_threshold =
        VOLTTHRESHOLD2DOUT(((float)knobs->soc_vm.thresholds[0].overvolt.alarm_threshold) / 2.0f);
    vm_alarm_setting2.alarmb_thresholds.hyst_threshold =
        VOLTTHRESHOLD2DOUT(((float)knobs->soc_vm.thresholds[0].undervolt.hyst_threshold) / 2.0f);
    vm_alarm_setting2.alarmb_thresholds.alarm_threshold =
        VOLTTHRESHOLD2DOUT(((float)knobs->soc_vm.thresholds[0].undervolt.alarm_threshold) / 2.0f);

    validate_vm_pvt(&pvt_config->vm_channels_config, &vm_alarm_setting, &vm_alarm_setting2, config->soc_vm, SOC_PVT_TOTAL_CHANNELS_VM);
}

void validate_dvfs_cfg(unsigned int core)
{
#define LOCK_TIMEOUT_60US 60
    dvfs_config_t dvfs_cfg = DVFS_DEFAULT_CONFIG;

    const power_knobs_t* knobs = &s_runconfig.knobs;
    const unsigned assigned_vft = s_runconfig.derived.assigned_vft[core];

    // fuse cfg
    dvfs_cfg.fuse_cfg.vft = &s_runconfig.dvfs_vft.curveset[assigned_vft].curve[0];
    // init cfg
    // configure default cppc settings
    dvfs_cfg.init_cfg.cppc.highest_perf = dvfs_get_cppc_from_pstate(s_runconfig.derived.vfts[assigned_vft].min_plimit);
    dvfs_cfg.init_cfg.cppc.lowest_perf = dvfs_get_cppc_from_pstate(MAX_PLIMIT);
    dvfs_cfg.init_cfg.cppc.nominal_perf = dvfs_get_cppc_from_pstate(DVFS_DEF_PLIMIT_INDEX_NOMINAL);
    dvfs_cfg.init_cfg.cppc.nominal_freq = dvfs_get_freq_from_plimit(DVFS_DEF_PLIMIT_INDEX_NOMINAL);
    dvfs_cfg.init_cfg.cppc.gtd_perf = dvfs_get_cppc_from_pstate(DVFS_DEF_PLIMIT_INDEX_NOMINAL);
    dvfs_cfg.init_cfg.cppc.lowest_nonlin_perf = dvfs_get_cppc_from_pstate(DVFS_DEF_PLIMIT_INDEX_NOMINAL);

    dvfs_cfg.init_cfg.adclk_enable = knobs->adclk_throt.enable;
    dvfs_cfg.init_cfg.pstate_telemetry_wr_address = s_telcfg.pstate_telem_wr_address;
    dvfs_cfg.init_cfg.scp_msg_telemetry_wr_address = s_telcfg.scp_msg_telem_wr_address;
    dvfs_cfg.init_cfg.sw_boot_mode = knobs->enable_survivability_mode;
    dvfs_pll_pstate_config_t pll_config;
    dvfs_pll_get_default_pstate_config(knobs->survivability_mode_pstate, &pll_config);

    // configure survivability mode
    dvfs_cfg.init_cfg.sw_boot_cfg.pll_lock_timeout_in_us = LOCK_TIMEOUT_60US;
    dvfs_cfg.init_cfg.sw_boot_cfg.dco_div_value = pll_config.dco_div_value;
    dvfs_cfg.init_cfg.sw_boot_cfg.freq_ctrl_value = pll_config.freq_ctrl_value;
    dvfs_cfg.init_cfg.sw_boot_cfg.dco_frac_value = pll_config.dco_frac_value;

    // pll config
    dvfs_cfg.init_cfg.calsm_enable = knobs->calsm_enable;
    dvfs_cfg.init_cfg.freq_table_cap_pstate =
        MAX(s_runconfig.derived.vfts[assigned_vft].min_plimit, knobs->fllcal_pstate_bounds.pstate_min);
    dvfs_cfg.init_cfg.freq_table_init = true;

    dvfs_cfg.static_pll_cfg.dco1_lckcntsel = knobs->plllock_cfg.lckcntsel;

    // throttle config
    dvfs_cfg.throttle_cfg.current_throttle.dec_amt0 = knobs->current_throt.dec_amt0;
    dvfs_cfg.throttle_cfg.current_throttle.dec_amt1 = knobs->current_throt.dec_amt1;
    dvfs_cfg.throttle_cfg.current_throttle.dec_ctr = knobs->current_throt.dec_ctr;
    dvfs_cfg.throttle_cfg.current_throttle.inc_amt = knobs->current_throt.inc_amt;
    dvfs_cfg.throttle_cfg.current_throttle.inc_ctr = knobs->current_throt.inc_ctr;
    dvfs_cfg.throttle_cfg.temp_throttle.inc_ctr = knobs->tile_temp_throt.inc_ctr;
    dvfs_cfg.throttle_cfg.temp_throttle.dec_ctr = knobs->tile_temp_throt.dec_ctr;
    dvfs_cfg.throttle_cfg.temp_throttle.inc_amt = knobs->tile_temp_throt.inc_amt;
    dvfs_cfg.throttle_cfg.temp_throttle.dec_amt = knobs->tile_temp_throt.dec_amt;
    dvfs_cfg.throttle_cfg.adclk_throttle.inc_ctr = knobs->adclk_throt.inc_ctr;
    dvfs_cfg.throttle_cfg.adclk_throttle.dec_ctr = knobs->adclk_throt.dec_ctr;
    dvfs_cfg.throttle_cfg.adclk_throttle.inc_amt = knobs->adclk_throt.inc_amt;
    dvfs_cfg.throttle_cfg.adclk_throttle.dec_amt = knobs->adclk_throt.dec_amt;

    // pll_adclk config
    dvfs_cfg.do_pll_adclk_config = knobs->adclk_throt.enable;
    dvfs_cfg.pll_adclk_cfg.resp_option = knobs->adclk_throt.resp_option;
    dvfs_cfg.pll_adclk_cfg.recovery_step_wait = knobs->adclk_throt.recovery_step_wait;
    dvfs_cfg.pll_adclk_cfg.recovery_step = knobs->adclk_throt.recovery_step;
    dvfs_cfg.pll_adclk_cfg.resp_wait = knobs->adclk_throt.resp_wait;
    dvfs_cfg.pll_adclk_cfg.resp_code = knobs->adclk_throt.resp_code;
    dvfs_cfg.pll_adclk_cfg.adclk_comp_config = knobs->adclk_throt.adclk_comp_config;
    dvfs_cfg.pll_adclk_cfg.throttle_threshold = knobs->adclk_throt.throttle_threshold;
    dvfs_cfg.pll_adclk_cfg.throttle_window_count = knobs->adclk_throt.throttle_window_count;

    // c1
    dvfs_cfg.init_cfg.c1_telem_en = knobs->c1_tel_enable;

    // TODO: enable ITD (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491054/)
    // (fix force_pstate vmat copy, full VFT generation before enabling)
    dvfs_cfg.init_cfg.pex_features.itd_en = 0;

    // adclk offset
    if (knobs->adclk_offset.enable)
    {
        dvfs_cfg.adclk_fuse_init = true;
        dvfs_cfg.fuse_cfg.adclk_offset = knobs->adclk_offset.offset_value[core];
    }

    assert_memory_equal(mock_dvfs_last_config(), &dvfs_cfg, sizeof(dvfs_cfg));
}

void validate_tile_pvt_telemetry(unsigned int core)
{
    tile_pvt_telem_setting_config_t tile_pvt_telem_cfg = PVT_TILE_TELEM_DEFAULT_CONFIG;

    tile_pvt_telem_cfg.temp_buffer_start_addr =
        s_telcfg.temp_telem_start_addr + ((core / 2) * s_telcfg.temp_telem_entry_size);
    tile_pvt_telem_cfg.volt_dma_settings.buffer_size = s_telcfg.volt_telem_buffer_size;
    tile_pvt_telem_cfg.volt_dma_settings.buffer_stride = s_telcfg.volt_telem_stride_size;

    tile_pvt_telem_cfg.volt_buffer_start_addr =
        s_telcfg.volt_telem_start_addr + ((core / 2) * s_telcfg.volt_telem_entry_size);
    tile_pvt_telem_cfg.temp_dma_settings.buffer_size = s_telcfg.temp_telem_buffer_size;
    tile_pvt_telem_cfg.temp_dma_settings.buffer_stride = s_telcfg.temp_telem_stride_size;

    assert_memory_equal(mock_pvt_last_tile_telem_config(), &tile_pvt_telem_cfg, sizeof(tile_pvt_telem_cfg));
}

static int setup(void** state)
{
    UNUSED(state);

    static const corebits_t default_cores = (const corebits_t)COREBITS_INIT_UINT32(0xFFFFFFFF, 0xFFFFFFFF, 0xF);

    base_config();
    base_telemetry();

    s_config.platform_soc_power_support = true;
    s_config.platform_core_power_support = true;
    s_config.platform_cores_in_die = &default_cores;
    s_config.platform_die_core_count = TEST_CORE_COUNT;

    return 0;
}

static int teardown(void** state)
{
    UNUSED(state);
    // ensure config pointer is restored
    s_runconfig.p_sconfig = &s_config;
    return 0;
}

void init_core_base_expect()
{
    expect_any_count(__wrap_wait_for_FLLCalDone, cluster_pex_base_addr, TEST_CORE_COUNT);
    will_return_always(__wrap_wait_for_FLLCalDone, DVFS_SUCCESS);

    expect_any_count(__wrap_dvfs_ns_set_cppc_desired2, cluster_pex_base_addr, TEST_CORE_COUNT);
    expect_value_count(__wrap_dvfs_ns_set_cppc_desired2,
                       cppc_desired,
                       dvfs_get_cppc_from_pstate(DVFS_DEF_PLIMIT_INDEX_NOMINAL),
                       TEST_CORE_COUNT);
    expect_value_count(__wrap_dvfs_ns_set_cppc_desired2,
                       cppc_base_perf,
                       dvfs_get_cppc_from_pstate(DVFS_DEF_PLIMIT_INDEX_NOMINAL),
                       TEST_CORE_COUNT);
    expect_value_count(__wrap_dvfs_ns_set_cppc_desired2, throttle_pri, 0, TEST_CORE_COUNT);
    expect_value_count(__wrap_dvfs_ns_set_cppc_desired2, boost_pri, 0, TEST_CORE_COUNT);

    will_return_count(__wrap_dvfs_init, DVFS_SUCCESS, TEST_CORE_COUNT);
    will_return_count(__wrap_odcm_init, ODCM_SUCCESS, TEST_CORE_COUNT);
    will_return_count(__wrap_tile_pvt_init, PVT_SUCCESS, (TEST_CORE_COUNT + 1) / 2);
}

void init_ws_core_base_expect(unsigned core_count, uint8_t pstate)
{
    expect_any_count(__wrap_reset_tile_pvt_dts_vm, cluster_pex_base_addr, (core_count + 1) / 2);
    expect_any_count(__wrap_tile_pvt_sda_reconfig, cluster_pex_base_addr, (core_count + 1) / 2);
    // fw mode should always be false
    expect_value_count(__wrap_tile_pvt_sda_reconfig, fw_sda_ip_control, false, (core_count + 1) / 2);
    will_return_always(__wrap_tile_pvt_sda_reconfig, PVT_SUCCESS);

    expect_any_count(__wrap_dvfs_telemetry_config, cluster_pex_base_addr, core_count);
    expect_any_count(__wrap_odcm_telemetry_config, cluster_pex_base_addr, core_count);
    expect_any_count(__wrap_tile_pvt_dma_config, cluster_pex_base_addr, (core_count + 1) / 2);

    expect_any_count(__wrap_dvfs_set_plimit, cluster_pex_base_addr, core_count);
    expect_value_count(__wrap_dvfs_set_plimit, plimit_index, pstate, core_count);
    expect_value_count(__wrap_dvfs_set_plimit, rack_power_cap, false, core_count);
}

#define ODCM_knob_CFG_SET(c1, value)  s_runconfig.knobs.current_throt.c1 = value;
#define ODCM_telem_CFG_SET(c1, value) s_telcfg.c1 = value;
#define ODCM_TEST_FULL(type, cfgname, cfgvalue)         \
    POWER_TEST(hwi_odcm_cfg_##cfgname, setup, teardown) \
    {                                                   \
                                                        \
        ODCM_##type##_CFG_SET(cfgname, cfgvalue);       \
        init_core_base_expect();                        \
        power_init_core(&s_runconfig, &s_telcfg);       \
        validate_odcm_cfg(TEST_CORE_COUNT - 1, false);  \
    }
#define ODCM_TEST(cfgname, cfgvalue)     ODCM_TEST_FULL(knob, cfgname, cfgvalue)
#define ODCM_TEST_TEL(cfgname, cfgvalue) ODCM_TEST_FULL(telem, cfgname, cfgvalue)

/*
    knobs->current_throt.telemetry_epoch_count   = 10;    // 10-20 valid
    knobs->current_throt.rolling_window_count    = 5;     // max 7
    knobs->current_throt.inc_ctr                 = 0x1f;  // 0x0-0x1f valid
    knobs->current_throt.dec_ctr                 = 0x1f;  // 0x0-0x1f valid
    knobs->current_throt.inc_amt                 = 0;     // 0-3 == 1-4
    knobs->current_throt.dec_amt0                = 1;     // 0-3 == 1-4
    knobs->current_throt.dec_amt1                = 2;     // 0-3 == 1-4
*/
// test change config of specific knobs to values other than above
ODCM_TEST(telemetry_epoch_count, 20);
ODCM_TEST(rolling_window_count, 3);
ODCM_TEST(inc_ctr, 2);
ODCM_TEST(dec_ctr, 2);
ODCM_TEST(inc_amt, 3);
ODCM_TEST(dec_amt0, 2);
ODCM_TEST(dec_amt1, 1);
ODCM_TEST_TEL(current_telem_start_addr, 0x100000u);
ODCM_TEST_TEL(current_telem_entry_size, 0x3u);

/* TILE PVT config tests */

#define PVTTILE_dts_CFG_SET(c1, c2, value) s_runconfig.knobs.tile_temp_throt.c1.c2 = value;
#define PVTTILE_vm_CFG_SET(c1, c2, value)                        \
    for (int idx = 0; idx < TILE_PVT_NUM_CHANNELS_VM; ++idx)     \
    {                                                            \
        s_runconfig.knobs.tile_vm.thresholds[idx].c1.c2 = value; \
    }
#define PVTTILE_telem_CFG_SET(c1, c2, value) s_telcfg.c2 = value;
#define PVTTILE_TEST_FULL(type, cfgname1, cfgname2, cfgvalue)                     \
    POWER_TEST(hwi_tilepvt_cfg_##type##_##cfgname1##_##cfgname2, setup, teardown) \
    {                                                                             \
                                                                                  \
        PVTTILE_##type##_CFG_SET(cfgname1, cfgname2, cfgvalue);                   \
        init_core_base_expect();                                                  \
        power_init_core(&s_runconfig, &s_telcfg);                                 \
        validate_tile_pvt_telemetry(TEST_CORE_COUNT - 1);                         \
        validate_tile_pvt_dts((TEST_CORE_COUNT - 1) / 2);                         \
        validate_tile_pvt_vm();                                                   \
    }
#define PVTTILE_TEST_DTS(area, threshold, cfgvalue) PVTTILE_TEST_FULL(dts, area, threshold, cfgvalue)
#define PVTTILE_TEST_VM(area, threshold, cfgvalue)  PVTTILE_TEST_FULL(vm, area, threshold, cfgvalue)
#define PVTTILE_TEST_TEL(cfgname, cfgvalue)         PVTTILE_TEST_FULL(telem, , cfgname, cfgvalue)

/*
    knobs->tile_vm.hyst_threshold  = 800;
    knobs->tile_vm.alarm_threshold = 600;

    knobs->tile_temp_throt.thermtrip.hyst_threshold  = 850;
    knobs->tile_temp_throt.thermtrip.alarm_threshold = 1050;
    knobs->tile_temp_throt.hot.hyst_threshold        = 900;
    knobs->tile_temp_throt.hot.alarm_threshold       = 950;
*/
// test change config of specific knobs to values other than above
PVTTILE_TEST_DTS(thermtrip, hyst_threshold, 900);
PVTTILE_TEST_DTS(thermtrip, alarm_threshold, 1100);
PVTTILE_TEST_DTS(hot, hyst_threshold, 950);
PVTTILE_TEST_DTS(hot, alarm_threshold, 1000);

PVTTILE_TEST_VM(overvolt, hyst_threshold, 700); // use values different than the valid defaults provided above
PVTTILE_TEST_VM(overvolt, alarm_threshold, 900); // for overvolt, hyst->alarm should be rising
PVTTILE_TEST_VM(undervolt, hyst_threshold, 900); // for undervolt, hyst->alarm should be dropping
PVTTILE_TEST_VM(undervolt, alarm_threshold, 700);

PVTTILE_TEST_TEL(temp_telem_start_addr, 0x100000);
PVTTILE_TEST_TEL(temp_telem_entry_size, 0x3);

PVTTILE_TEST_TEL(volt_telem_start_addr, 0x100000);
PVTTILE_TEST_TEL(volt_telem_entry_size, 0x3);

/* SOC PVT config tests */

#define PVTSOC_dts_CFG_SET(c1, c2, value) s_runconfig.knobs.soc_temp.c1.c2 = value;
#define PVTSOC_vm_CFG_SET(c1, c2, value)                        \
    for (int idx = 0; idx < SOC_PVT_TOTAL_CHANNELS_VM; ++idx)   \
    {                                                           \
        s_runconfig.knobs.soc_vm.thresholds[idx].c1.c2 = value; \
    }
#define PVTSOC_TEST_FULL(type, cfgname1, cfgname2, cfgvalue)                     \
    POWER_TEST(hwi_socpvt_cfg_##type##_##cfgname1##_##cfgname2, setup, teardown) \
    {                                                                            \
                                                                                 \
        PVTSOC_##type##_CFG_SET(cfgname1, cfgname2, cfgvalue);                   \
        will_return(__wrap_soc_pvt_init, PVT_SUCCESS);                           \
        power_init_soc(&s_runconfig);                                            \
        validate_soc_pvt_dts();                                                  \
        validate_soc_pvt_vm();                                                   \
    }
#define PVTSOC_TEST_DTS(area, threshold, cfgvalue) PVTSOC_TEST_FULL(dts, area, threshold, cfgvalue)
#define PVTSOC_TEST_VM(area, threshold, cfgvalue)  PVTSOC_TEST_FULL(vm, area, threshold, cfgvalue)

/*
    knobs->soc_temp.thermtrip.hyst_threshold  = 850;
    knobs->soc_temp.thermtrip.alarm_threshold = 1000;
    knobs->soc_temp.hot.hyst_threshold        = 850;
    knobs->soc_temp.hot.alarm_threshold       = 900;

    knobs->soc_vm.hyst_threshold  = 800;
    knobs->soc_vm.alarm_threshold = 600;
*/
// test change config of specific knobs to values other than above
PVTSOC_TEST_DTS(thermtrip, hyst_threshold, 900);
PVTSOC_TEST_DTS(thermtrip, alarm_threshold, 1100);
PVTSOC_TEST_DTS(hot, hyst_threshold, 950);
PVTSOC_TEST_DTS(hot, alarm_threshold, 1000);
PVTSOC_TEST_VM(overvolt, hyst_threshold, 700);  // use values different than the valid defaults provided above
PVTSOC_TEST_VM(overvolt, alarm_threshold, 900); // for overvolt, hyst->alarm should be rising
PVTSOC_TEST_VM(undervolt, hyst_threshold, 900); // for undervolt, hyst->alarm should be dropping
PVTSOC_TEST_VM(undervolt, alarm_threshold, 700);

/* TILE PVT config tests */

#define DVFS_knob_CFG_SET(c1, c2, value)  s_runconfig.knobs.c1.c2 = value;
#define DVFS_telem_CFG_SET(c1, c2, value) s_telcfg.c2 = value;
#define DVFS_TEST_FULL(type, cfgname1, cfgname2, cfgvalue)                     \
    POWER_TEST(hwi_dvfs_cfg_##type##_##cfgname1##_##cfgname2, setup, teardown) \
    {                                                                          \
                                                                               \
        DVFS_##type##_CFG_SET(cfgname1, cfgname2, cfgvalue);                   \
        init_core_base_expect();                                               \
        power_init_core(&s_runconfig, &s_telcfg);                              \
        validate_dvfs_cfg(TEST_CORE_COUNT - 1);                                \
    }
#define DVFS_TEST(area, threshold, cfgvalue) DVFS_TEST_FULL(knob, area, threshold, cfgvalue)
#define DVFS_TEST_TEL(cfgname, cfgvalue)     DVFS_TEST_FULL(telem, , cfgname, cfgvalue)

/*
    knobs->current_throt.inc_ctr                 = 0x1f;  // 0x0-0x1f valid
    knobs->current_throt.dec_ctr                 = 0x1f;  // 0x0-0x1f valid
    knobs->current_throt.inc_amt                 = 0;     // 0-3 == 1-4
    knobs->current_throt.dec_amt0                = 1;     // 0-3 == 1-4
    knobs->current_throt.dec_amt1                = 2;     // 0-3 == 1-4

    knobs->adclk_throt.enable  = false;  // disable until further investigation done
    knobs->adclk_throt.inc_ctr = 0x1f;   // 0x0-0x1f valid
    knobs->adclk_throt.dec_ctr = 0x1f;   // 0x0-0x1f valid
    knobs->adclk_throt.inc_amt = 0;      // 0-3 == 1-4
    knobs->adclk_throt.dec_amt = 1;      // 0-3 == 1-4

    knobs->tile_temp_throt.inc_ctr                   = 0xf;  // 0x0-0xf valid
    knobs->tile_temp_throt.dec_ctr                   = 0xf;  // 0x0-0xf valid
    knobs->tile_temp_throt.inc_amt                   = 0;    // 0-3 == 1-4
    knobs->tile_temp_throt.dec_amt                   = 1;    // 0-3 == 1-4
*/
// test change config of specific knobs to values other than above
DVFS_TEST(current_throt, inc_ctr, 3);
DVFS_TEST(current_throt, dec_ctr, 7);
DVFS_TEST(current_throt, inc_amt, 3);
DVFS_TEST(current_throt, dec_amt0, 2);
DVFS_TEST(current_throt, dec_amt1, 1);

DVFS_TEST(adclk_throt, enable, true);
DVFS_TEST(adclk_throt, inc_ctr, 7);
DVFS_TEST(adclk_throt, dec_ctr, 3);
DVFS_TEST(adclk_throt, inc_amt, 2);
DVFS_TEST(adclk_throt, dec_amt, 0);

DVFS_TEST(tile_temp_throt, inc_ctr, 7);
DVFS_TEST(tile_temp_throt, dec_ctr, 3);
DVFS_TEST(tile_temp_throt, inc_amt, 2);
DVFS_TEST(tile_temp_throt, dec_amt, 0);

DVFS_TEST_TEL(pstate_telem_wr_address, 0x100000);
DVFS_TEST_TEL(scp_msg_telem_wr_address, 0x100000);

#define TEST_FORCE_PSTATE 5

/* main init core test; expect calls to all init APIs based on core count, return of success */
POWER_TEST(hwi_init_core, setup, teardown)
{

    // this is the default expectation setup for running init_core
    init_core_base_expect();
    // run core init
    power_init_core(&s_runconfig, &s_telcfg);
    // verify odcm config
    validate_odcm_cfg(TEST_CORE_COUNT - 1, false);
    // verify tile pvt config
    validate_tile_pvt_telemetry(TEST_CORE_COUNT - 1);
    validate_tile_pvt_dts((TEST_CORE_COUNT - 1) / 2);
    validate_tile_pvt_vm();
    // verify dvfs config
    validate_dvfs_cfg(TEST_CORE_COUNT - 1);
}

/* main init core test; expect calls to all init APIs based on core count, return of success */
POWER_TEST(hwi_init_core__alternate_dts_coeff, setup, teardown)
{

    power_fuse_data_t* f = &s_runconfig.fuses;

    // this is the default expectation setup for running init_core
    init_core_base_expect();
    // now update last tile k_val
    f->dts_coeff_tile[(TEST_CORE_COUNT - 1) / 2].k_val = 290;
    // run core init
    power_init_core(&s_runconfig, &s_telcfg);
    // verify odcm config
    validate_odcm_cfg(TEST_CORE_COUNT - 1, false);
    // verify tile pvt config
    validate_tile_pvt_telemetry(TEST_CORE_COUNT - 1);
    validate_tile_pvt_dts((TEST_CORE_COUNT - 1) / 2); // <- can only validate last
    validate_tile_pvt_vm();
    // verify dvfs config
    validate_dvfs_cfg(TEST_CORE_COUNT - 1);
}

/* init ws core test; expect calls to all telemetry init APIs based on core count */
POWER_TEST(hwi_init_ws_core, setup, teardown)
{

    // this is the default expectation setup for running init_core
    init_ws_core_base_expect(TEST_CORE_COUNT, MAX_PLIMIT);
    // run core init
    power_init_ws_core(&s_runconfig, &s_telcfg);
    // verify odcm config
    validate_odcm_cfg(TEST_CORE_COUNT - 1, true);
    // verify tile pvt config
    validate_tile_pvt_telemetry(TEST_CORE_COUNT - 1);
}

/* init ws core test; expect calls to all telemetry init APIs based on core count */
POWER_TEST(hwi_init_ws_core__disabled_core, setup, teardown)
{
    // disable core 1
    static const corebits_t disabled_core = (const corebits_t)COREBITS_INIT_UINT32(0xFFFFFFFD, 0xFFFFFFFF, 0xF);

    s_config.platform_cores_in_die = &disabled_core;
    // increase core count, still expect same result because core 1 is disabled
    s_config.platform_die_core_count = TEST_CORE_COUNT + 1;

    // this is the default expectation setup for running init_core
    init_ws_core_base_expect(TEST_CORE_COUNT, MAX_PLIMIT);
    // run core init
    power_init_ws_core(&s_runconfig, &s_telcfg);
    // verify odcm config
    validate_odcm_cfg(TEST_CORE_COUNT - 1, true);
    // verify tile pvt config
    validate_tile_pvt_telemetry(TEST_CORE_COUNT - 1);
}

/* init ws core test; expect calls to all telemetry init APIs based on core count */
POWER_TEST(hwi_init_ws_core__force_pstate, setup, teardown)
{

    s_runconfig.knobs.force_pstate = TEST_FORCE_PSTATE;

    // this is the default expectation setup for running init_core
    init_ws_core_base_expect(1, MAX_PLIMIT);
    // run core init
    power_init_ws_core(&s_runconfig, &s_telcfg);
    // verify odcm config
    validate_odcm_cfg(0, true);
    // verify tile pvt config
    validate_tile_pvt_telemetry(0);
}

/* init ws core test; HW not supported - expect no calls to telemetry config functions */
POWER_TEST(hwi_init_ws_core__not_supported, setup, teardown)
{

    s_config.platform_core_power_support = false;
    // run core init
    power_init_ws_core(&s_runconfig, &s_telcfg);
}

/* core forced pstate test; expect pll and vmat config updated */
POWER_TEST(hwi_init_core__forced_pstate, setup, teardown)
{
#define TEST_FORCED_PSTATE 0x5

    // this is the default expectation setup for running init_core (for 1 core)
    power_knobs_t* knobs = &s_runconfig.knobs;

    expect_any_count(__wrap_wait_for_FLLCalDone, cluster_pex_base_addr, 1);
    will_return_always(__wrap_wait_for_FLLCalDone, DVFS_SUCCESS);

    expect_any(__wrap_dvfs_ns_set_cppc_desired2, cluster_pex_base_addr);
    expect_value(__wrap_dvfs_ns_set_cppc_desired2, cppc_desired, dvfs_get_cppc_from_pstate(DVFS_DEF_PLIMIT_INDEX_NOMINAL));
    expect_value(__wrap_dvfs_ns_set_cppc_desired2, cppc_base_perf, dvfs_get_cppc_from_pstate(DVFS_DEF_PLIMIT_INDEX_NOMINAL));
    expect_value(__wrap_dvfs_ns_set_cppc_desired2, throttle_pri, 0);
    expect_value(__wrap_dvfs_ns_set_cppc_desired2, boost_pri, 0);

    will_return_count(__wrap_dvfs_init, DVFS_SUCCESS, 1);
    will_return_count(__wrap_odcm_init, ODCM_SUCCESS, 1);
    will_return_count(__wrap_tile_pvt_init, PVT_SUCCESS, 1);

    fgpll_reg fake_regs = {};

    // setup scp_exp_csr_base such that our fgpll_reg is at the appropriate offset
    power_service_config_t test_config;
    test_config = s_config;
    test_config.cluster_pex_base = (uintptr_t)(&fake_regs) - PEX_CORE_PLL_ADDRESS;
    s_runconfig.p_sconfig = &test_config;

    const ptr_fgpll_ftable_cr1_index0 ftable_cr1 = &fake_regs.ftable_cr1_index0;
    const ptr_fgpll_ftable_cr2_index0 ftable_cr2 = &fake_regs.ftable_cr2_index0;

    // unique per-pstate data
    for (unsigned pstate_idx = 0; pstate_idx < NUM_PSTATES; ++pstate_idx)
    {
        dvfs_pll_pstate_config_t pll_config;
        dvfs_pll_get_default_pstate_config(pstate_idx, &pll_config);

        ftable_cr1[pstate_idx].dco0frac = pll_config.dco_frac_value;
        ftable_cr2[pstate_idx].freqctrl = pll_config.freq_ctrl_value;
        ftable_cr2[pstate_idx].dco0div = pll_config.dco_div_value;
    }

    // only one core's worth of fake core regs, so core0 has to be the one manipulated
    knobs->force_pstate = TEST_FORCED_PSTATE;

    // run core init
    power_init_core(&s_runconfig, &s_telcfg);

    const dvfs_vft_t* vft = mock_dvfs_last_vft_config();

    // verify non-unique values for everything forced and lower
    for (unsigned pstate_idx = TEST_FORCED_PSTATE + 1; pstate_idx < NUM_PSTATES; ++pstate_idx)
    {
        assert_int_equal(ftable_cr1[pstate_idx].dco0frac, ftable_cr1[TEST_FORCED_PSTATE].dco0frac);
        assert_int_equal(ftable_cr2[pstate_idx].freqctrl, ftable_cr2[TEST_FORCED_PSTATE].freqctrl);
        assert_int_equal(ftable_cr2[pstate_idx].dco0div, ftable_cr2[TEST_FORCED_PSTATE].dco0div);

        assert_int_equal(vft->vmat_info[0].ldo_dac_in[pstate_idx], vft->vmat_info[0].ldo_dac_in[TEST_FORCED_PSTATE]);
        assert_int_equal(vft->vmat_info[0].memasst_hd_ema[pstate_idx], vft->vmat_info[0].memasst_hd_ema[TEST_FORCED_PSTATE]);
        assert_int_equal(vft->vmat_info[0].memasst_hd_rawlm[pstate_idx],
                         vft->vmat_info[0].memasst_hd_rawlm[TEST_FORCED_PSTATE]);
        assert_int_equal(vft->vmat_info[0].memasst_hshc_ema[pstate_idx],
                         vft->vmat_info[0].memasst_hshc_ema[TEST_FORCED_PSTATE]);
        assert_int_equal(vft->vmat_info[0].memasst_hshc_rawlm[pstate_idx],
                         vft->vmat_info[0].memasst_hshc_rawlm[TEST_FORCED_PSTATE]);
        assert_int_equal(vft->vmat_info[0].memasst_tp_emaa[pstate_idx],
                         vft->vmat_info[0].memasst_tp_emaa[TEST_FORCED_PSTATE]);
        assert_int_equal(vft->vmat_info[0].memasst_tp_emab[pstate_idx],
                         vft->vmat_info[0].memasst_tp_emab[TEST_FORCED_PSTATE]);
        assert_int_equal(vft->vmat_info[0].memasst_hd_emaw[pstate_idx],
                         vft->vmat_info[0].memasst_hd_emaw[TEST_FORCED_PSTATE]);
    }
}

/* main init core test; expect calls to all init APIs based on core count, return of success */
// can't run this test with only one core.
#if (TEST_CORE_COUNT > 1)
POWER_TEST(hwi_init_core__disabled_core, setup, teardown)
{

    // mark core 3 as invalid/disabled
    corebits_clear_bit(&s_runconfig.fuses.valid_cores, 3);

    // this is the default expectation setup for running init_core, updated for disabled core

    expect_any_count(__wrap_wait_for_FLLCalDone, cluster_pex_base_addr, TEST_CORE_COUNT - 1);
    will_return_always(__wrap_wait_for_FLLCalDone, DVFS_SUCCESS);

    expect_any_count(__wrap_dvfs_ns_set_cppc_desired2, cluster_pex_base_addr, TEST_CORE_COUNT - 1);
    expect_value_count(__wrap_dvfs_ns_set_cppc_desired2,
                       cppc_desired,
                       dvfs_get_cppc_from_pstate(DVFS_DEF_PLIMIT_INDEX_NOMINAL),
                       TEST_CORE_COUNT - 1);
    expect_value_count(__wrap_dvfs_ns_set_cppc_desired2,
                       cppc_base_perf,
                       dvfs_get_cppc_from_pstate(DVFS_DEF_PLIMIT_INDEX_NOMINAL),
                       TEST_CORE_COUNT - 1);
    expect_value_count(__wrap_dvfs_ns_set_cppc_desired2, throttle_pri, 0, TEST_CORE_COUNT - 1);
    expect_value_count(__wrap_dvfs_ns_set_cppc_desired2, boost_pri, 0, TEST_CORE_COUNT - 1);

    will_return_count(__wrap_dvfs_init, DVFS_SUCCESS, TEST_CORE_COUNT);
    will_return_count(__wrap_odcm_init, ODCM_SUCCESS, TEST_CORE_COUNT - 1);
    will_return_count(__wrap_tile_pvt_init, PVT_SUCCESS, TEST_CORE_COUNT / 2);

    // run core init
    power_init_core(&s_runconfig, &s_telcfg);
    // verify odcm config
    assert_int_equal(mock_dvfs_last_config()->fuse_cfg.core_disabled, true);
}
#endif

POWER_TEST(hwi_init_core__mpmm_enable, setup, teardown)
{

    power_knobs_t* knobs = &s_runconfig.knobs;
    knobs->mpmm.enable = true;

    // this is the default expectation setup for running init_core
    init_core_base_expect();
    // run core init
    power_init_core(&s_runconfig, &s_telcfg);
}

POWER_TEST(hwi_init_core__mpmm_gear, setup, teardown)
{

    power_knobs_t* knobs = &s_runconfig.knobs;
    knobs->mpmm.enable = true;
    knobs->mpmm.gear = 1;

    // this is the default expectation setup for running init_core
    init_core_base_expect();
    // run core init
    power_init_core(&s_runconfig, &s_telcfg);
}

POWER_TEST(hwi_init_core__c1_telem_enable, setup, teardown)
{

    power_knobs_t* knobs = &s_runconfig.knobs;
    knobs->c1_tel_enable = true;

    // this is the default expectation setup for running init_core
    init_core_base_expect();
    // run core init
    power_init_core(&s_runconfig, &s_telcfg);
    validate_dvfs_cfg(TEST_CORE_COUNT - 1);
}

POWER_TEST(hwi_init_core__c1_telem_disable, setup, teardown)
{

    power_knobs_t* knobs = &s_runconfig.knobs;
    knobs->c1_tel_enable = false;

    // this is the default expectation setup for running init_core
    init_core_base_expect();
    // run core init
    power_init_core(&s_runconfig, &s_telcfg);
    validate_dvfs_cfg(TEST_CORE_COUNT - 1);
}

POWER_TEST(hwi_init_core__cap_freq_to_min_plimit, setup, teardown)
{
#define TEST_MIN_PLIMIT 14

    // update min_plimit of last core
    s_runconfig.derived.vfts[s_runconfig.derived.assigned_vft[TEST_CORE_COUNT - 1]].min_plimit = TEST_MIN_PLIMIT;

    // this is the default expectation setup for running init_core
    init_core_base_expect();
    // run core init
    power_init_core(&s_runconfig, &s_telcfg);
    // verify dvfs config - will verify correct limit used for freq cap
    validate_dvfs_cfg(TEST_CORE_COUNT - 1);
}

POWER_TEST(hwi_init_core__adclk_enable, setup, teardown)
{

    // verify output with adclk enabled
    power_knobs_t* knobs = &s_runconfig.knobs;
    knobs->adclk_offset.enable = true;

    // this is the default expectation setup for running init_core
    init_core_base_expect();
    // run core init
    power_init_core(&s_runconfig, &s_telcfg);
}

POWER_TEST(hwi_init_core__core_disabled, setup, teardown)
{

    // disable core 1
    static const corebits_t disabled_core = (const corebits_t)COREBITS_INIT_UINT32(0xFFFFFFFD, 0xFFFFFFFF, 0xF);

    s_config.platform_cores_in_die = &disabled_core;
    // increase core count, still expect same result because core 1 is disabled
    s_config.platform_die_core_count = TEST_CORE_COUNT + 1;

    // this is the default expectation setup for running init_core
    init_core_base_expect();
    // run core init
    power_init_core(&s_runconfig, &s_telcfg);
}

/* init core test, provide invalid vft curve for second core */
#ifdef BUGCHECK_TESTS
POWER_TEST(hwi_init_core__dvfs_vft, setup, teardown)
{
    // setup second core to fail due to no valid vf curve
    s_runconfig.derived.vfts[1].min_plimit = NUM_PSTATES;

    // this is the default expectation setup for running init_core

    will_return_count(__wrap_dvfs_init, DVFS_SUCCESS, 1);
    will_return_count(__wrap_odcm_init, ODCM_SUCCESS, 2);
    will_return_count(__wrap_tile_pvt_init, PVT_SUCCESS, 1);

    expect_value(__wrap_CrashDumpBugCheck, errorCode, (uint32_t)CC_SC_CORE_DVFS_CFG_INIT);

    // Call API under test, handling noreturn
    if (!bugcheck_mock_return())
    {
        // run core init
        power_init_core(&s_runconfig, &s_telcfg);
    }
}

/* failing init core test; expect 1 call to init APIs based on failure location, return of state error */
POWER_TEST(hwi_init_core__odcm_fail, setup, teardown)
{

    will_return_count(__wrap_odcm_init, ODCM_NULL_PARAM, 1);

    expect_value(__wrap_CrashDumpBugCheck, errorCode, (uint32_t)CC_SC_CORE_ODCM_INIT);

    // Call API under test, handling noreturn
    if (!bugcheck_mock_return())
    {
        // run core init
        power_init_core(&s_runconfig, &s_telcfg);
    }
}

/* failing init core test; expect 1 call to init APIs based on failure location, return of state error */
POWER_TEST(hwi_init_core__dvfs_fail, setup, teardown)
{

    will_return_count(__wrap_odcm_init, ODCM_SUCCESS, 1);
    will_return_count(__wrap_dvfs_init, DVFS_NULL_PARAM, 1);

    expect_value(__wrap_CrashDumpBugCheck, errorCode, (uint32_t)CC_SC_CORE_DVFS_INIT);

    // Call API under test, handling noreturn
    if (!bugcheck_mock_return())
    {
        // run core init
        power_init_core(&s_runconfig, &s_telcfg);
    }
}

/* failing init core test; expect 1 call to init APIs based on failure location, return of state error */
POWER_TEST(hwi_init_core__pvt_fail, setup, teardown)
{

    will_return_count(__wrap_odcm_init, ODCM_SUCCESS, 1);
    will_return_count(__wrap_dvfs_init, DVFS_SUCCESS, 1);
    will_return_count(__wrap_tile_pvt_init, PVT_NULL_PARAM, 1);

    expect_value(__wrap_CrashDumpBugCheck, errorCode, (uint32_t)CC_SC_TILE_PVT_INIT);

    // Call API under test, handling noreturn
    if (!bugcheck_mock_return())
    {
        // run core init
        power_init_core(&s_runconfig, &s_telcfg);
    }
}

/* init core test that fails fll cal wait */
POWER_TEST(hwi_init_core__fll_wait_fail, setup, teardown)
{

    // mark core 3 as invalid/disabled
    corebits_clear_bit(&s_runconfig.fuses.valid_cores, 3);

    // this is the default expectation setup for running init_core, updated for disabled core

    expect_any_count(__wrap_wait_for_FLLCalDone, cluster_pex_base_addr, 1);
    will_return_always(__wrap_wait_for_FLLCalDone, DVFS_TIMEOUT);

    expect_any(__wrap_dvfs_ns_set_cppc_desired2, cluster_pex_base_addr);
    expect_value(__wrap_dvfs_ns_set_cppc_desired2, cppc_desired, dvfs_get_cppc_from_pstate(DVFS_DEF_PLIMIT_INDEX_NOMINAL));
    expect_value(__wrap_dvfs_ns_set_cppc_desired2, cppc_base_perf, dvfs_get_cppc_from_pstate(DVFS_DEF_PLIMIT_INDEX_NOMINAL));
    expect_value(__wrap_dvfs_ns_set_cppc_desired2, throttle_pri, 0);
    expect_value(__wrap_dvfs_ns_set_cppc_desired2, boost_pri, 0);

    will_return_count(__wrap_dvfs_init, DVFS_SUCCESS, TEST_CORE_COUNT);
    will_return_count(__wrap_odcm_init, ODCM_SUCCESS, TEST_CORE_COUNT - 1);
    will_return_count(__wrap_tile_pvt_init, PVT_SUCCESS, TEST_CORE_COUNT / 2);

    expect_value(__wrap_CrashDumpBugCheck, errorCode, (uint32_t)CC_SC_CORE_WAIT_FLL_CAL);

    // Call API under test, handling noreturn
    if (!bugcheck_mock_return())
    {
        // run core init
        power_init_core(&s_runconfig, &s_telcfg);
    }
}

/* failing init soc test; expect call up to failing init APIs, return of state error */
POWER_TEST(hwi_init_soc__pvt_fail, setup, teardown)
{

    will_return(__wrap_soc_pvt_init, PVT_NULL_PARAM);

    expect_value(__wrap_CrashDumpBugCheck, errorCode, (uint32_t)CC_SC_SOC_PVT_INIT);

    // Call API under test, handling noreturn
    if (!bugcheck_mock_return())
    {
        // run soc init
        power_init_soc(&s_runconfig);
    }
}

#endif

/* main init soc test; expect calls to all init APIs, return of success */
POWER_TEST(hwi_init_soc, setup, teardown)
{

    will_return(__wrap_soc_pvt_init, PVT_SUCCESS);

    // run soc init
    power_init_soc(&s_runconfig);
    // check soc pvt config
    validate_soc_pvt_dts();
    validate_soc_pvt_vm();
}

POWER_TEST(hwi_power_set_plimit, setup, teardown)
{
#define TEST_PLIMIT 18

    power_service_config_t config;
    dvfs_plimit plimit = {.vf_index = TEST_PLIMIT, .power_cap = true};

    s_runconfig.p_sconfig = &config;
    config.cluster_pex_base = 0x1000;
    config.cluster_stride = 1;
    unsigned int core = 5;
    const uintptr_t cluster_pex_base_addr = (config.cluster_pex_base + (config.cluster_stride * core));

    expect_value(__wrap_dvfs_config_plimit, cluster_pex_base_addr, cluster_pex_base_addr);
    expect_value(__wrap_dvfs_config_plimit, plimit_val, plimit.as_uint32);

    power_set_plimit(&s_runconfig, core, plimit);

    expect_value(__wrap_dvfs_config_plimit, cluster_pex_base_addr, cluster_pex_base_addr);
    plimit.power_cap = false;
    expect_value(__wrap_dvfs_config_plimit, plimit_val, plimit.as_uint32);

    power_set_plimit(&s_runconfig, core, plimit);
    s_runconfig.p_sconfig = NULL;
}

POWER_TEST(power_hw_pstate_from_freq__exact, setup, teardown)
{
#define TEST_PSTATE 5

    uint16_t test_freq = dvfs_get_freq_from_plimit(TEST_PSTATE);
    uint8_t test_pstate = power_hw_pstate_from_freq(test_freq);

    // should get an exact match
    assert_int_equal(test_pstate, TEST_PSTATE);
}

POWER_TEST(power_hw_pstate_from_freq__next, setup, teardown)
{
#define TEST_PSTATE 5

    // subtract one from freq for test pstate
    uint16_t test_freq = dvfs_get_freq_from_plimit(TEST_PSTATE) - 1;
    uint8_t test_pstate = power_hw_pstate_from_freq(test_freq);

    // should get an next pstate
    assert_int_equal(test_pstate, TEST_PSTATE + 1);
}

POWER_TEST(power_hw_pstate_from_freq__no_match, setup, teardown)
{
    // subtract one from freq of minimum perf
    uint16_t test_freq = dvfs_get_freq_from_plimit(MAX_PLIMIT) - 1;
    uint8_t test_pstate = power_hw_pstate_from_freq(test_freq);

    // should get MAX_PLIMIT
    assert_int_equal(test_pstate, MAX_PLIMIT);
}

POWER_TEST(power_hw_get_adclk_count, setup, teardown)
{
#define TEST_DROOP_COUNT 12345

    expect_value(__wrap_dvfs_get_adclk_droop_count, cluster_pex_base_addr, s_runconfig.p_sconfig->cluster_pex_base);
    will_return(__wrap_dvfs_get_adclk_droop_count, TEST_DROOP_COUNT);
    will_return(__wrap_dvfs_get_adclk_droop_count,
                DVFS_SUCCESS); // doesn't matter what we put here, since return from this function is ignored

    assert_int_equal(power_hw_get_adclk_count(&s_runconfig, 0), TEST_DROOP_COUNT);
}

POWER_TEST(power_hw_force_pmin, setup, teardown)
{
    scp_exp_csr_reg fake_reg = {{{0}}};

    power_service_config_t sconfig = {.scp_exp_csr_base = (uintptr_t) & (fake_reg)};
    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};

    scp_exp_csr_force_pmin_reg expected_pmin_reg = {{.fw_pmin_control = 1}};
    will_return(__wrap_power_runconfig_get, &test_runconfig);

    power_hw_force_pmin(PM_FW_PMIN_CONTROL);

    assert_int_equal(fake_reg.force_pmin_reg.fw_pmin_control, expected_pmin_reg.fw_pmin_control);

    expected_pmin_reg.lockup_ue_rr = 1;

    will_return(__wrap_power_runconfig_get, &test_runconfig);
    power_hw_force_pmin(PM_LOCKUP_UE_RR);

    assert_int_equal(fake_reg.force_pmin_reg.lockup_ue_rr, expected_pmin_reg.lockup_ue_rr);
}

POWER_TEST(power_hw_clear_force_pmin, setup, teardown)
{
    scp_exp_csr_reg fake_reg = {{{0}}};
    fake_reg.force_pmin_reg.fw_pmin_control = 1;
    fake_reg.force_pmin_reg.lockup_ue_rr = 1;

    power_service_config_t sconfig = {.scp_exp_csr_base = (uintptr_t) & (fake_reg)};
    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};

    scp_exp_csr_force_pmin_reg expected_pmin_reg = {{.fw_pmin_control = 0}};
    will_return(__wrap_power_runconfig_get, &test_runconfig);

    power_hw_clear_force_pmin(PM_FW_PMIN_CONTROL);

    assert_int_equal(fake_reg.force_pmin_reg.fw_pmin_control, expected_pmin_reg.fw_pmin_control);

    expected_pmin_reg.lockup_ue_rr = 0;

    will_return(__wrap_power_runconfig_get, &test_runconfig);
    power_hw_clear_force_pmin(PM_LOCKUP_UE_RR);

    assert_int_equal(fake_reg.force_pmin_reg.lockup_ue_rr, expected_pmin_reg.lockup_ue_rr);
}

POWER_TEST(power_vrs_write_vcpu_voltage, setup, teardown)
{
    scp_avs_device_t test_avs_device = {
        .avs_bus_num = AVS_BUS0,
    };
    static avs_pwr_request_context_t test_avs_Request;

    power_service_config_t sconfig = {};
    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};
    uint16_t test_volt_mv = 995;
    uint8_t vcpu_test_index = 0;

    test_avs_Request.request.avs_response_single_resp.error.as_uint8 = AVS_ERROR_NONE;
    test_avs_Request.in_use = 1;
    test_avs_Request.request.avs_params.avs_cmd_array[0].rail_id = 0;
    test_avs_Request.request.avs_params.avs_cmd_array[0].cmd_type = AVS_VOLTAGE_RW;
    test_avs_Request.request.avs_params.avs_data = (uint32_t)test_volt_mv;

    sconfig.avs_details[vcpu_test_index].bus_id = vcpu_test_index;
    sconfig.avs_details[vcpu_test_index].rail_id = vcpu_test_index;

    will_return(__wrap_power_runconfig_get, &test_runconfig);

    expect_function_call(__wrap_all_requests_completed);
    expect_value(__wrap_scp_avs_client_write, Interface, sconfig.avs_details[vcpu_test_index].bus_id);
    expect_memory(__wrap_scp_avs_client_write, Request, &test_avs_Request, sizeof(test_avs_Request));
    expect_value(__wrap_scp_avs_client_write, CompletionRoutine, AVSPwrWriteRequestCompletion);
    expect_any(__wrap_scp_avs_client_write, CompletionContext);
    expect_function_call(__wrap_scp_avs_client_write);

    power_vrs_write_vcpu_voltage(test_volt_mv);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
}

POWER_TEST(power_vrs_write_vcpu_voltage_high_range, setup, teardown)
{
    scp_avs_device_t test_avs_device = {
        .avs_bus_num = AVS_BUS0,
    };
    static avs_pwr_request_context_t test_avs_Request;

    power_service_config_t sconfig = {};
    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};
    uint16_t test_volt_mv = VR_VCPU_MAX_VOLTAGE_MV + 1;
    uint8_t vcpu_test_index = 0;

    test_avs_Request.request.avs_response_single_resp.error.as_uint8 = AVS_ERROR_NONE;
    test_avs_Request.in_use = 1;
    test_avs_Request.request.avs_params.avs_cmd_array[0].rail_id = 0;
    test_avs_Request.request.avs_params.avs_cmd_array[0].cmd_type = AVS_VOLTAGE_RW;
    test_avs_Request.request.avs_params.avs_data = (uint32_t)VR_VCPU_MAX_VOLTAGE_MV;

    sconfig.avs_details[vcpu_test_index].bus_id = vcpu_test_index;
    sconfig.avs_details[vcpu_test_index].rail_id = vcpu_test_index;

    will_return(__wrap_power_runconfig_get, &test_runconfig);

    expect_function_call(__wrap_all_requests_completed);
    expect_value(__wrap_scp_avs_client_write, Interface, sconfig.avs_details[vcpu_test_index].bus_id);
    expect_memory(__wrap_scp_avs_client_write, Request, &test_avs_Request, sizeof(test_avs_Request));
    expect_any(__wrap_scp_avs_client_write, CompletionRoutine);
    expect_any(__wrap_scp_avs_client_write, CompletionContext);
    expect_function_call(__wrap_scp_avs_client_write);

    power_vrs_write_vcpu_voltage(test_volt_mv);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
}

POWER_TEST(power_vrs_write_vcpu_voltage_low_range, setup, teardown)
{
    scp_avs_device_t test_avs_device = {
        .avs_bus_num = AVS_BUS0,
    };
    static avs_pwr_request_context_t test_avs_Request[MAX_AVS_INST] = {};

    power_service_config_t sconfig = {};
    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};
    uint16_t test_volt_mv = VR_VCPU_MIN_VOLTAGE_MV - 1;
    uint8_t vcpu_test_index = 0;

    test_avs_Request[test_avs_device.avs_bus_num].request.avs_response_single_resp.error.as_uint8 = AVS_ERROR_NONE;
    test_avs_Request[test_avs_device.avs_bus_num].in_use = 1;
    test_avs_Request[test_avs_device.avs_bus_num].request.avs_params.avs_cmd_array[0].rail_id = 0;
    test_avs_Request[test_avs_device.avs_bus_num].request.avs_params.avs_cmd_array[0].cmd_type = AVS_VOLTAGE_RW;
    test_avs_Request[test_avs_device.avs_bus_num].request.avs_params.avs_data = (uint32_t)VR_VCPU_MIN_VOLTAGE_MV;

    sconfig.avs_details[vcpu_test_index].bus_id = vcpu_test_index;
    sconfig.avs_details[vcpu_test_index].rail_id = vcpu_test_index;

    will_return(__wrap_power_runconfig_get, &test_runconfig);

    expect_function_call(__wrap_all_requests_completed);
    expect_value(__wrap_scp_avs_client_write, Interface, sconfig.avs_details[vcpu_test_index].bus_id);
    expect_memory(__wrap_scp_avs_client_write, Request, &test_avs_Request, sizeof(test_avs_Request));
    expect_any(__wrap_scp_avs_client_write, CompletionRoutine);
    expect_any(__wrap_scp_avs_client_write, CompletionContext);
    expect_function_call(__wrap_scp_avs_client_write);

    power_vrs_write_vcpu_voltage(test_volt_mv);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
}

POWER_TEST(power_vrs_write_vcpu_voltage_callback, setup, teardown)
{
    PDFWK_ASYNC_REQUEST_HEADER request = NULL;
    scp_avs_device_t test_avs_device = {
        .avs_bus_num = AVS_BUS0,
    };

    pwr_avs_request[test_avs_device.avs_bus_num].request.avs_response_single_resp.error.as_uint8 = AVS_ERROR_NONE;
    pwr_avs_request[test_avs_device.avs_bus_num].request.avs_response_single_resp.error.v_done = 1;
    pwr_avs_request[test_avs_device.avs_bus_num].request.avs_response_status = SCP_AVS_STATUS_SUCCESS;
    pwr_avs_request[test_avs_device.avs_bus_num].request.Header.RequestType = AVS_REQUEST_WRITE_DATA;
    pwr_avs_request[test_avs_device.avs_bus_num].in_use = 1;
    pwr_avs_request[test_avs_device.avs_bus_num].request.avs_params.avs_cmd_array[0].rail_id = 0;
    pwr_avs_request[test_avs_device.avs_bus_num].request.avs_params.avs_cmd_array[0].cmd_type = AVS_VOLTAGE_RW;

    AVSPwrWriteRequestCompletion(request, (void*)(int)test_avs_device.avs_bus_num);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
    assert_int_equal(pwr_avs_request[test_avs_device.avs_bus_num].in_use, 0);
}

POWER_TEST(power_vrs_write_vcpu_voltage_callback_error, setup, teardown)
{
    PDFWK_ASYNC_REQUEST_HEADER request = NULL;
    scp_avs_device_t test_avs_device = {
        .avs_bus_num = AVS_BUS0,
    };
    pwr_avs_request[test_avs_device.avs_bus_num].request.avs_response_status = SCP_AVS_STATUS_WRITE_FAIL;
    pwr_avs_request[test_avs_device.avs_bus_num].in_use = 1;

    AVSPwrWriteRequestCompletion(request, (void*)(int)test_avs_device.avs_bus_num);
    assert_int_equal(pwr_avs_request[test_avs_device.avs_bus_num].in_use, 0);
}

POWER_TEST(power_vrs_read_vcpu_voltage_callback, setup, teardown)
{
    PDFWK_ASYNC_REQUEST_HEADER request = NULL;
    scp_avs_device_t test_avs_device = {
        .avs_bus_num = AVS_BUS0,
    };

    pwr_avs_request[test_avs_device.avs_bus_num].request.avs_response_single_resp.error.as_uint8 = AVS_ERROR_NONE;
    pwr_avs_request[test_avs_device.avs_bus_num].request.avs_response_single_resp.error.v_done = 1;
    pwr_avs_request[test_avs_device.avs_bus_num].request.avs_response_status = SCP_AVS_STATUS_SUCCESS;
    pwr_avs_request[test_avs_device.avs_bus_num].request.Header.RequestType = AVS_REQUEST_READ_DATA;
    pwr_avs_request[test_avs_device.avs_bus_num].in_use = 1;
    pwr_avs_request[test_avs_device.avs_bus_num].request.avs_params.avs_cmd_array[0].rail_id = 0;
    pwr_avs_request[test_avs_device.avs_bus_num].request.avs_params.avs_cmd_array[0].cmd_type = AVS_VOLTAGE_RW;

    AVSPwrReadRequestCompletion(request, (void*)(int)test_avs_device.avs_bus_num);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS0);
    assert_int_equal(pwr_avs_request[test_avs_device.avs_bus_num].in_use, 0);
}

POWER_TEST(power_vrs_read_vcpu_voltage_callback_error, setup, teardown)
{
    PDFWK_ASYNC_REQUEST_HEADER request = NULL;
    scp_avs_device_t test_avs_device = {
        .avs_bus_num = AVS_BUS0,
    };
    pwr_avs_request[test_avs_device.avs_bus_num].request.avs_response_status = SCP_AVS_STATUS_WRITE_FAIL;
    pwr_avs_request[test_avs_device.avs_bus_num].in_use = 1;

    AVSPwrReadRequestCompletion(request, (void*)(int)test_avs_device.avs_bus_num);
    assert_int_equal(pwr_avs_request[test_avs_device.avs_bus_num].in_use, 0);
}
