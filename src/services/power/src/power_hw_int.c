//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_hw_int.c
 * Implementation of power hw initialization functions
 */

/*------------- Includes -----------------*/
// clang-format off
#include <kng_soc_constants.h> // for NUM_AP_CORES_PER_DIE, NUM_CPU_TILES
// clang-format on

#include "dvfs_struct.h"     // for dvfs_config_t, dvfs_init_config_t
#include "odcm_struct.h"     // for odcm_config_t, odcm_telem_config_t
#include "power_hw_int_i.h"  // for power_telcfg_t, power_hw_dts_pvt_raw_...
#include "power_i.h"         // for TEMP2DOUT_FUSED, BUG_CHECK
#include "power_runconfig.h" // for power_knobs_t, dts_coeff_t, power_run...
#include "power_runconfig_i.h"

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <bug_check.h>  // for BUG_CHECK
#include <corebits.h>   // for corebits_is_bit_set
#include <dvfs.h>       // for dvfs_get_cppc_from_pstate, dvfs_pll_g...
#include <dvfs_regs.h>  // for (anonymous union)::(anonymous), dvfs_...
#include <fgpll_regs.h> // for fgpll_pll_error_mask_cr, (anonymous u...
#include <ift_fw.h>     // for ift_is_enabled
#include <kng_error.h>
#include <odcm.h>         // for odcm_init, odcm_telemetry_config, ODC...
#include <pex_regs.h>     // for PEX_CORE_PLL_ADDRESS
#include <power_events.h> // for POWER_ET_FATAL, POWER_ET_TYPE_CURVE_H...
#include <pvt.h>          // for PVT_SUCCESS, reset_tile_pvt_dts_vm
#include <pvt_struct.h>   // for pvt_alarm_setting_config_t, pvt_thres...
#include <scp_exp_csr_regs.h>
#include <sensor_thresholds.h> // for SOC_TMP_MAX_HIGH_CRITICAL_DC
#include <silibs_common.h>     // for ARRAY_SIZE, MAX
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdbool.h>     // for bool, true, false
#include <stddef.h>      // for NULL
#include <stdint.h>      // for UINT16_MAX, uint16_t, uint8_t, uintptr_t
#include <string.h>      // for memcpy
#include <system_info.h> // for system_info_is_warm_start
#include <utils.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void core_pll_mask_error_sr(const power_runconfig_t* p_runconfig, int core);

/*-- Declarations (Statics and globals) --*/
// PVT dout helper macros
#define CLIP_PVT(dout) (uint16_t)(dout > UINT16_MAX ? UINT16_MAX : dout)
// temperature thresholds are in 0.1C increments, divide by 10 to get value for dout
#define TEMPTHRESHOLD2DOUT(threshold, fuses) \
    CLIP_PVT(TEMP2DOUT_FUSED(((float)threshold / 10.0F), fuses.k_val, fuses.y_val))
// voltage thresholds are in mV increments, divide by 1000 to get value for dout
#define VOLTTHRESHOLD2DOUT(threshold, div2) \
    CLIP_PVT(VOLTS2DOUT((float)threshold / (div2 ? 2000.0F : 1000.0F)))

// Define an FLL cal timeout
#define FLL_CAL_TIMEOUT_US          1200
#define FLL_CAL_TIMEOUT_FPGA_US     5000
#define SW_BOOT_PLL_LOCK_TIMEOUT_US 60

// Define settings structures large enough for both SOC and tile PVT config
#define MAX_CHANNELS(a, b)   (a > b ? a : b)
#define MAX_PVT_VM_CHANNELS  MAX_CHANNELS(SOC_PVT_TOTAL_CHANNELS_VM, TILE_PVT_NUM_CHANNELS_VM)
#define MAX_PVT_DTS_CHANNELS MAX_CHANNELS(SOC_PVT_TOTAL_CHANNELS_DTS, TILE_PVT_NUM_CHANNELS_DTS)

pvt_alarm_setting_config_t s_pvt_vm_alarm_settings[MAX_PVT_VM_CHANNELS] = {0};
pvt_alarm_setting_config_t s_pvt_dts_alarm_settings[MAX_PVT_DTS_CHANNELS] = {0};

/*------------- Functions ----------------*/

/**
 * @brief Copies PLL config from starting pstate to next pstates
 *
 * \b Description:
 *      For a given core (cluster_pex_base address) and pstate,
 *      copies the PLL config of the initial pstate to the following lower
 *      performance pstates - this is for force_pstate knob.
 *
 * @param[in] cluster_pex_base_addr - core/cluster register base address
 * @param[in] pstate - pstate to retrieve PLL config for
 *
 * @return None
 *
 */
static void pll_copy_to_min(uintptr_t cluster_pex_base_addr, uint8_t pstate)
{
    FPFW_RUNTIME_ASSERT(pstate < NUM_PSTATES);
    dvfs_pll_pstate_config_t source_pll_config;
    int status = dvfs_pll_get_default_pstate_config(pstate, &source_pll_config);
    FPFW_RUNTIME_ASSERT(status == DVFS_SUCCESS);
    for (unsigned pstate_idx = pstate + 1; pstate_idx < NUM_PSTATES; ++pstate_idx)
    {
        status = dvfs_pll_set_pstate_config(cluster_pex_base_addr, pstate_idx, &source_pll_config);
        FPFW_RUNTIME_ASSERT(status == DVFS_SUCCESS);
    }
}

void setup_forced_pstate(uintptr_t cluster_pex_base_addr, dvfs_config_t* dvfs_cfg, dvfs_vft_t* temp_vft, unsigned int core, uint8_t pstate)
{
    FPFW_RUNTIME_ASSERT(dvfs_cfg != NULL);
    FPFW_RUNTIME_ASSERT(temp_vft != NULL);
    FPFW_RUNTIME_ASSERT(pstate < NUM_PSTATES);
    FPFW_RUNTIME_ASSERT(core < NUM_AP_CORES_PER_DIE);

    // first copy pll config from the forced pstate down to P_max, so regardless of pstate, freq will be the same below this point
    pll_copy_to_min(cluster_pex_base_addr, pstate);

    // set initial plimit to control loop initialized limit for forced pstate
    dvfs_cfg->init_cfg.plimit_index = MAX_PLIMIT;
    // do not initialize ftable (as it would replace action of pll_copy_to_min above)
    dvfs_cfg->init_cfg.freq_table_init = false;

    // duplicate the assigned vft to temp
    memcpy(temp_vft, dvfs_cfg->fuse_cfg.vft, sizeof(dvfs_vft_t));
    // reassign to copy
    dvfs_cfg->fuse_cfg.vft = temp_vft;

    uint8_t min_ldo_dac_idx = 0;
    uint8_t crv_idx = 1;

    if (dvfs_cfg->init_cfg.pex_features.itd_en == 0)
    {
        while (crv_idx < NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS)
        {
            if (temp_vft->vmat_info[min_ldo_dac_idx].ldo_dac_in[pstate] < temp_vft->vmat_info[crv_idx].ldo_dac_in[pstate])
            {
                min_ldo_dac_idx = crv_idx;
            }
            crv_idx++;
        }
    }

    for (unsigned pstate_idx = pstate + 1; pstate_idx < NUM_PSTATES; ++pstate_idx)
    {
        for (crv_idx = 0; crv_idx < VFT_CURVE_COUNT_PER_CURVESET; ++crv_idx)
        {
            uint8_t ldo_dac_idx = (dvfs_cfg->init_cfg.pex_features.itd_en == 0) ? min_ldo_dac_idx : crv_idx;

            temp_vft->vmat_info[crv_idx].ldo_dac_in[pstate_idx] = temp_vft->vmat_info[ldo_dac_idx].ldo_dac_in[pstate];
            temp_vft->vmat_info[crv_idx].memasst_hd_ema[pstate_idx] =
                temp_vft->vmat_info[ldo_dac_idx].memasst_hd_ema[pstate];
            temp_vft->vmat_info[crv_idx].memasst_hd_rawlm[pstate_idx] =
                temp_vft->vmat_info[ldo_dac_idx].memasst_hd_rawlm[pstate];
            temp_vft->vmat_info[crv_idx].memasst_hshc_ema[pstate_idx] =
                temp_vft->vmat_info[ldo_dac_idx].memasst_hshc_ema[pstate];
            temp_vft->vmat_info[crv_idx].memasst_hshc_rawlm[pstate_idx] =
                temp_vft->vmat_info[ldo_dac_idx].memasst_hshc_rawlm[pstate];
            temp_vft->vmat_info[crv_idx].memasst_tp_emaa[pstate_idx] =
                temp_vft->vmat_info[ldo_dac_idx].memasst_tp_emaa[pstate];
            temp_vft->vmat_info[crv_idx].memasst_tp_emab[pstate_idx] =
                temp_vft->vmat_info[ldo_dac_idx].memasst_tp_emab[pstate];
        }
    }
}

/**
 * @brief Updates odcm config structure
 *
 * \b Description:
 *      Updates odcm config structure with common config based on p_knobs
 *
 * @param[inout] odcm_cfg - odcm config structure
 *
 * @return None
 *
 */
static PLACED_CODE void power_init_update_odcm_cfg_common(odcm_config_t* odcm_cfg, const power_knobs_t* p_knobs)
{
    FPFW_RUNTIME_ASSERT(odcm_cfg != NULL);
    FPFW_RUNTIME_ASSERT(p_knobs != NULL);

    // ensure fuse config is disabled as fuses are auto-propagated
    odcm_cfg->do_fuse_config = false;
    odcm_cfg->mma_cfg.rolling_window_count =
        p_knobs->current_throt.rolling_window_count; // Number of samples for current rolling window average. ex: 2^5 = 32 samples
    odcm_cfg->mma_cfg.epoch_count =
        p_knobs->current_throt.telemetry_epoch_count; // Size of epoch (2^n) counts for current samples before sending out telemetry
}

/**
 * @brief Updates odcm config structure
 *
 * \b Description:
 *      Updates odcm config structure with config specific to a core
 *
 * @param[inout] odcm_cfg - odcm config structure
 * @param[in] p_telemetry_config - telemetry config
 * @param[in] core - specific core
 *
 * @return None
 *
 */
static PLACED_CODE void power_init_update_odcm_cfg_core(odcm_telem_config_t* odcm_telem_cfg,
                                                        const power_telcfg_t* p_telemetry_config,
                                                        unsigned int core)
{
    FPFW_RUNTIME_ASSERT(odcm_telem_cfg != NULL);
    FPFW_RUNTIME_ASSERT(p_telemetry_config != NULL);

    odcm_telem_cfg->buffer_start_addr =
        p_telemetry_config->current_telem_start_addr + (core * p_telemetry_config->current_telem_entry_size);
    odcm_telem_cfg->buffer_size = p_telemetry_config->current_telem_buffer_size;
    odcm_telem_cfg->buffer_stride = p_telemetry_config->current_telem_stride_size;
}

/**
 * @brief Updates dvfs config structure
 *
 * \b Description:
 *      Updates dvfs config structure for diabled core
 *
 * @param[inout] dvfs_cfg - dvfs config structure
 *
 * @return None
 *
 */
static void power_init_update_dvfs_cfg_disable(dvfs_config_t* dvfs_cfg)
{
    FPFW_RUNTIME_ASSERT(dvfs_cfg != NULL);
    dvfs_cfg->fuse_cfg.core_disabled = true;
}

/**
 * @brief Updates dvfs config structure
 *
 * \b Description:
 *      Updates dvfs config structure with common config based on p_knobs
 *
 * @param[in] p_runconfig - power runtime config
 * @param[inout] dvfs_cfg - dvfs config structure
 * @param[in] p_telemetry_config - telemetry config
 *
 * @return None
 *
 */
static PLACED_CODE void power_init_update_dvfs_cfg_common(const power_runconfig_t* p_runconfig,
                                                          dvfs_config_t* p_dvfs_cfg,
                                                          const power_telcfg_t* p_telemetry_config)
{
    FPFW_RUNTIME_ASSERT(p_dvfs_cfg != NULL);
    FPFW_RUNTIME_ASSERT(p_telemetry_config != NULL);
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    const power_knobs_t* p_knobs = &p_runconfig->knobs;

    // configure telemetry addresses
    p_dvfs_cfg->init_cfg.pstate_telemetry_wr_address = p_telemetry_config->pstate_telem_wr_address;
    p_dvfs_cfg->init_cfg.scp_msg_telemetry_wr_address = p_telemetry_config->scp_msg_telem_wr_address;
    // update current throttle config based on p_knobs
    p_dvfs_cfg->throttle_cfg.current_throttle.dec_amt0 = p_knobs->current_throt.dec_amt0;
    p_dvfs_cfg->throttle_cfg.current_throttle.dec_amt1 = p_knobs->current_throt.dec_amt1;
    p_dvfs_cfg->throttle_cfg.current_throttle.dec_ctr = p_knobs->current_throt.dec_ctr;
    p_dvfs_cfg->throttle_cfg.current_throttle.inc_amt = p_knobs->current_throt.inc_amt;
    p_dvfs_cfg->throttle_cfg.current_throttle.inc_ctr = p_knobs->current_throt.inc_ctr;
    // update temp throttle config based on p_knobs
    p_dvfs_cfg->throttle_cfg.temp_throttle.inc_ctr = p_knobs->tile_temp_throt.inc_ctr;
    p_dvfs_cfg->throttle_cfg.temp_throttle.dec_ctr = p_knobs->tile_temp_throt.dec_ctr;
    p_dvfs_cfg->throttle_cfg.temp_throttle.inc_amt = p_knobs->tile_temp_throt.inc_amt;
    p_dvfs_cfg->throttle_cfg.temp_throttle.dec_amt = p_knobs->tile_temp_throt.dec_amt;
    // update adclk config based on p_knobs
    p_dvfs_cfg->init_cfg.adclk_enable = p_knobs->adclk_throt.enable;
    p_dvfs_cfg->throttle_cfg.adclk_throttle.inc_ctr = p_knobs->adclk_throt.inc_ctr;
    p_dvfs_cfg->throttle_cfg.adclk_throttle.dec_ctr = p_knobs->adclk_throt.dec_ctr;
    p_dvfs_cfg->throttle_cfg.adclk_throttle.inc_amt = p_knobs->adclk_throt.inc_amt;
    p_dvfs_cfg->throttle_cfg.adclk_throttle.dec_amt = p_knobs->adclk_throt.dec_amt;
    p_dvfs_cfg->do_pll_adclk_config = p_knobs->adclk_throt.enable;
    p_dvfs_cfg->pll_adclk_cfg.resp_option = p_knobs->adclk_throt.resp_option;
    p_dvfs_cfg->pll_adclk_cfg.recovery_step_wait = p_knobs->adclk_throt.recovery_step_wait;
    p_dvfs_cfg->pll_adclk_cfg.recovery_step = p_knobs->adclk_throt.recovery_step;
    p_dvfs_cfg->pll_adclk_cfg.resp_wait = p_knobs->adclk_throt.resp_wait;
    p_dvfs_cfg->pll_adclk_cfg.resp_code = p_knobs->adclk_throt.resp_code;
    p_dvfs_cfg->pll_adclk_cfg.adclk_comp_config = p_knobs->adclk_throt.adclk_comp_config;
    p_dvfs_cfg->pll_adclk_cfg.throttle_threshold = p_knobs->adclk_throt.throttle_threshold;
    p_dvfs_cfg->pll_adclk_cfg.throttle_window_count = p_knobs->adclk_throt.throttle_window_count;
    p_dvfs_cfg->pll_adclk_cfg.input_sense_enable = p_knobs->adclk_throt.input_sense_enable;
    // get the default pstate config for the survivability mode
    dvfs_pll_pstate_config_t pll_config;
    int status = dvfs_pll_get_default_pstate_config(p_knobs->survivability_mode_pstate, &pll_config);
    FPFW_RUNTIME_ASSERT(status == DVFS_SUCCESS);
    // configure survivability mode
    p_dvfs_cfg->init_cfg.sw_boot_mode = p_knobs->enable_survivability_mode;
    p_dvfs_cfg->init_cfg.sw_boot_cfg.pll_lock_timeout_in_us = SW_BOOT_PLL_LOCK_TIMEOUT_US;
    p_dvfs_cfg->init_cfg.sw_boot_cfg.dco_div_value = pll_config.dco_div_value;
    p_dvfs_cfg->init_cfg.sw_boot_cfg.freq_ctrl_value = pll_config.freq_ctrl_value;
    p_dvfs_cfg->init_cfg.sw_boot_cfg.dco_frac_value = pll_config.dco_frac_value;
    // configure calsm settings
    p_dvfs_cfg->init_cfg.calsm_enable = p_knobs->calsm_enable;
    // configure default cppc settings
    p_dvfs_cfg->init_cfg.cppc.lowest_perf = dvfs_get_cppc_from_pstate(MAX_PLIMIT);
    p_dvfs_cfg->init_cfg.cppc.nominal_perf = dvfs_get_cppc_from_pstate(p_runconfig->derived.pnominal);
    p_dvfs_cfg->init_cfg.cppc.nominal_freq = dvfs_get_freq_from_plimit(p_runconfig->derived.pnominal);
    p_dvfs_cfg->init_cfg.cppc.gtd_perf = dvfs_get_cppc_from_pstate(p_runconfig->derived.pnominal);
    p_dvfs_cfg->init_cfg.cppc.lowest_freq = dvfs_get_freq_from_plimit(MAX_PLIMIT);

    // config static pll
    p_dvfs_cfg->static_pll_cfg.dco1_lckcntsel = p_knobs->plllock_cfg.lckcntsel;
    // enable c1 telemetry based on knob
    p_dvfs_cfg->init_cfg.c1_telem_en = p_knobs->c1_tel_enable;
    p_dvfs_cfg->init_cfg.pex_features.itd_en = p_knobs->itd_cfg;
    p_dvfs_cfg->init_cfg.vmat_log_en = p_knobs->vmat_log_en;
    p_dvfs_cfg->init_cfg.fcal_log_en = p_knobs->fcal_log_en;
}

static unsigned find_lowest_nonlin_pstate_idx(const power_runconfig_t* p_runconfig, const power_core_vft_t* p_vft)
{
    FPFW_RUNTIME_ASSERT(p_vft != NULL);
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    // start with voltage from lowest point
    unsigned lowest_nonlin = MAX_PLIMIT;
    uint16_t voltage = p_vft->vf[MAX_PLIMIT].voltage_mv;
    // only search up to nominal
    for (unsigned vf_idx = MAX_PLIMIT - 1; vf_idx >= (unsigned)p_runconfig->derived.pnominal; vf_idx--)
    {
        if (p_vft->vf[vf_idx].voltage_mv == voltage)
        {
            lowest_nonlin = vf_idx;
        }
        else
        {
            // once we have voltage change, we're at the lowest non-linear point
            break;
        }
    }
    return lowest_nonlin;
}

/**
 * @brief Updates dvfs config structure
 *
 * \b Description:
 *      Updates dvfs config structure specific to core
 *
 * @param[in] p_runconfig - power runtime config
 * @param[inout] p_dvfs_cfg - dvfs config structure
 * @param[in] core - specific core
 *
 * @return FWK_SUCCESS or FWK_E_DEVICE
 *
 */
static PLACED_CODE void power_init_update_dvfs_cfg_core(const power_runconfig_t* p_runconfig,
                                                        dvfs_config_t* p_dvfs_cfg,
                                                        unsigned int core)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);
    FPFW_RUNTIME_ASSERT(p_dvfs_cfg != NULL);

    const power_knobs_t* p_knobs = &p_runconfig->knobs;
    const unsigned assigned_vft = p_runconfig->derived.assigned_vft[core];
    p_dvfs_cfg->fuse_cfg.vft = &p_runconfig->dvfs_vft.curveset[assigned_vft];

    if (p_runconfig->derived.vfts[assigned_vft].min_plimit == NUM_PSTATES)
    {
        // curve has no supported pstates
        POWER_LOG_CRIT("Assigned VF curve %d for core %d has no valid pstates", assigned_vft, core);

        POWER_ET_FATAL(POWER_ET_TYPE_CURVE_HAS_NO_VALID_PSTATES, 0, POWER_ET_ENCODE_DETAIL_CORE(assigned_vft, core));

        BUG_CHECK(KNG_SC_NO_VALID_PSTATES, core, assigned_vft);
    }
    p_dvfs_cfg->fuse_cfg.core_disabled = false;

    uint16_t temp_itd_temp_boundaries[NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS - 1] = {0};

    const uint32_t tile_num = core / CORES_PER_TILE;
    for (uint8_t temp_idx = 0; temp_idx < sizeof(p_runconfig->fuses.curve_max_temp); ++temp_idx)
    {
        temp_itd_temp_boundaries[temp_idx] =
            (uint16_t)CLIP_PVT(TEMP2DOUT_FUSED(((float)p_runconfig->fuses.curve_max_temp[temp_idx]),
                                               p_runconfig->fuses.dts_coeff_tile[tile_num].k_val,
                                               p_runconfig->fuses.dts_coeff_tile[tile_num].y_val));
    }

    memcpy((void*)p_dvfs_cfg->fuse_cfg.vft->itd_temp_boundaries,
           temp_itd_temp_boundaries,
           sizeof(uint16_t) * (NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS - 1));

    //! Print just to cross-check the temp boundaries
    for (uint8_t temp_idx = 0; temp_idx < sizeof(p_runconfig->fuses.curve_max_temp); ++temp_idx)
    {
        uint16_t temp = power_hw_dts_pvt_raw_to_temp_dC(temp_itd_temp_boundaries[temp_idx],
                                                        p_runconfig->fuses.dts_coeff_tile[tile_num]);
        POWER_LOG_INFO("Temp idx %d, Temp dC:[%d] C:[%d]", temp_idx, temp, temp / 10);
    }

    // update highest perf based on curve
    p_dvfs_cfg->init_cfg.cppc.highest_perf = dvfs_get_cppc_from_pstate(p_runconfig->derived.vfts[assigned_vft].min_plimit);

    p_dvfs_cfg->init_cfg.cppc.lowest_nonlin_perf =
        (p_runconfig->knobs.cppc_lowest_nonlin_perf < NUM_PSTATES)
            ? dvfs_get_cppc_from_pstate(p_runconfig->knobs.cppc_lowest_nonlin_perf)
            : dvfs_get_cppc_from_pstate(find_lowest_nonlin_pstate_idx(p_runconfig, &p_runconfig->derived.vfts[assigned_vft]));

    // update initial plimit based on min_plimit
    const uint8_t pnominal = p_runconfig->derived.pnominal;
    p_dvfs_cfg->init_cfg.plimit_index =
        p_runconfig->knobs.allowed_plimit_minimum > pnominal ? p_runconfig->knobs.allowed_plimit_minimum : pnominal;
    // set max freq in table (mainly for calibration) to the minimum perf level of either the knob or the
    // selected curve (MAX as plimit decreases with higher perf)
    p_dvfs_cfg->init_cfg.freq_table_cap_pstate =
        MAX(p_knobs->fllcal_pstate_bounds.pstate_min, p_runconfig->derived.vfts[assigned_vft].min_plimit);

    // initialize ftable - may be cleared by force pstate
    p_dvfs_cfg->init_cfg.freq_table_init = true;

    // handle adclk offset if knob enabled
    if (p_knobs->adclk_offset.enable)
    {
        p_dvfs_cfg->adclk_fuse_init = true;
        p_dvfs_cfg->fuse_cfg.adclk_offset = p_knobs->adclk_offset.offset_value[core];
    }

    // Pass the fuses.memasst pointer to dvfs_cfg
    p_dvfs_cfg->fuse_cfg.memasst_entries = &(p_runconfig->fuses.memasst);
}

/**
 * @brief Updates pvt_config structure
 *
 * \b Description:
 *      Updates tile pvt config based on p_knobs/fuses
 *
 * @param[in] p_runconfig - power runtime config
 * @param[inout] pvt_cfg - pvt config structure
 *
 * @return None
 *
 */
static PLACED_CODE void power_init_update_tilepvt_cfg(const power_runconfig_t* p_runconfig, pvt_setting_config_t* pvt_cfg)
{
    FPFW_RUNTIME_ASSERT(pvt_cfg != NULL);
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    const power_knobs_t* p_knobs = &p_runconfig->knobs;

    // ensure match between config and HW
    static_assert(NUM_TILE_VM == TILE_PVT_NUM_CHANNELS_VM, "Mismatch between HW library and config on tile VM count");
    static_assert(ARRAY_SIZE(p_knobs->tile_vm.thresholds) == TILE_PVT_NUM_CHANNELS_VM,
                  "Mismatch between HW library and config manager on tile VM count");
    pvt_cfg->vm_channels_config.setting_count = TILE_PVT_NUM_CHANNELS_VM;
    pvt_cfg->vm_channels_config.alarm_settings = (pvt_alarm_setting_config_t*)&s_pvt_vm_alarm_settings;
    for (int vm_idx = 0; vm_idx < TILE_PVT_NUM_CHANNELS_VM; ++vm_idx)
    {
        const bool div_by_2 = ((p_runconfig->p_sconfig->tile_vm[vm_idx].flags & VM_FLAGS_DIV2) != 0);
        pvt_alarm_setting_config_t alarm_setting = {0};
        alarm_setting.alarma_enable = p_knobs->tile_vm_overvolt_en;
        alarm_setting.alarmb_enable = p_knobs->tile_vm_undervolt_en;
        alarm_setting.alarma_thresholds.hyst_threshold =
            VOLTTHRESHOLD2DOUT(p_knobs->tile_vm.thresholds[vm_idx].overvolt.hyst_threshold, div_by_2);
        alarm_setting.alarma_thresholds.alarm_threshold =
            VOLTTHRESHOLD2DOUT(p_knobs->tile_vm.thresholds[vm_idx].overvolt.alarm_threshold, div_by_2);
        alarm_setting.alarmb_thresholds.hyst_threshold =
            VOLTTHRESHOLD2DOUT(p_knobs->tile_vm.thresholds[vm_idx].undervolt.hyst_threshold, div_by_2);
        alarm_setting.alarmb_thresholds.alarm_threshold =
            VOLTTHRESHOLD2DOUT(p_knobs->tile_vm.thresholds[vm_idx].undervolt.alarm_threshold, div_by_2);
        s_pvt_vm_alarm_settings[vm_idx] = alarm_setting;
    }
}

/**
 * @brief Updates pvt_config structure per tile
 *
 * \b Description:
 *      Updates tile pvt config based on p_knobs/fuses
 *
 * @param[in] p_runconfig - power runtime config
 * @param[inout] pvt_cfg - pvt config structure
 * @param[inout] tile - tile number
 *
 * @return None
 *
 */
static PLACED_CODE void power_init_update_tilepvt_tile_cfg(const power_runconfig_t* p_runconfig,
                                                           pvt_setting_config_t* pvt_cfg,
                                                           unsigned int tile)
{
    FPFW_RUNTIME_ASSERT(pvt_cfg != NULL);
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    const power_knobs_t* p_knobs = &p_runconfig->knobs;
    const power_fuse_data_t* fuses = &p_runconfig->fuses;

    pvt_cfg->dts_channels_config.setting_count = TILE_PVT_NUM_CHANNELS_DTS;
    pvt_cfg->dts_channels_config.alarm_settings = (pvt_alarm_setting_config_t*)&s_pvt_dts_alarm_settings;
    for (int dts_idx = 0; dts_idx < TILE_PVT_NUM_CHANNELS_DTS; ++dts_idx)
    {
        pvt_alarm_setting_config_t alarm_setting = {0};
        alarm_setting.alarma_enable = p_knobs->tile_temp_hot_en;
        alarm_setting.alarmb_enable = p_knobs->tile_temp_thermtrip_en;
        alarm_setting.alarma_thresholds.hyst_threshold =
            TEMPTHRESHOLD2DOUT(p_knobs->tile_temp_throt.hot.hyst_threshold, fuses->dts_coeff_tile[tile]);
        alarm_setting.alarma_thresholds.alarm_threshold =
            TEMPTHRESHOLD2DOUT(p_knobs->tile_temp_throt.hot.alarm_threshold, fuses->dts_coeff_tile[tile]);
        alarm_setting.alarmb_thresholds.hyst_threshold =
            TEMPTHRESHOLD2DOUT(p_knobs->tile_temp_throt.thermtrip.hyst_threshold, fuses->dts_coeff_tile[tile]);
        alarm_setting.alarmb_thresholds.alarm_threshold =
            TEMPTHRESHOLD2DOUT(p_knobs->tile_temp_throt.thermtrip.alarm_threshold, fuses->dts_coeff_tile[tile]);
        s_pvt_dts_alarm_settings[dts_idx] = alarm_setting;
    }
}

uint16_t power_hw_dts_pvt_raw_to_temp_dC(uint16_t raw, dts_coeff_t fused_coeff)
{
    // Convert raw DTS output to deci-degrees Celsius with rounding
    uint16_t result = (uint16_t)FLOAT_TO_UNSIGNED(DOUT2TEMP_FUSED(raw, fused_coeff.k_val, fused_coeff.y_val) * 10);

    // Check if temperature is outside normal operating range, skip for SVP as the DTS raw & coefficients may not be populated
    if ((!IS_PLATFORM_SVP()) && ((result == 0) || (result >= SOC_TMP_MAX_HIGH_CRITICAL_DC)))
    {
        POWER_ET_WARN(POWER_ET_TYPE_DTS_TEMP_OUT_OF_RANGE, result);
    }

    return result;
}

/**
 * @brief Updates pvt_config structure
 *
 * \b Description:
 *      Updates tile pvt telemetry config from telemetry detail
 *
 * @param[inout] pvt_telem_cfg - pvt telemetry config structure
 * @param[in] p_telemetry_config - telemetry config
 * @param[in] tile - tile config is for
 *
 * @return None
 *
 */
static PLACED_CODE void power_init_update_tilepvt_telemetry_cfg(tile_pvt_telem_setting_config_t* pvt_telem_cfg,
                                                                const power_telcfg_t* p_telemetry_config,
                                                                unsigned int tile)
{
    FPFW_RUNTIME_ASSERT(pvt_telem_cfg != NULL);
    FPFW_RUNTIME_ASSERT(p_telemetry_config != NULL);

    pvt_telem_cfg->temp_buffer_start_addr =
        p_telemetry_config->temp_telem_start_addr + (tile * p_telemetry_config->temp_telem_entry_size);
    pvt_telem_cfg->volt_buffer_start_addr =
        p_telemetry_config->volt_telem_start_addr + (tile * p_telemetry_config->volt_telem_entry_size);
    pvt_telem_cfg->temp_dma_settings.buffer_size = p_telemetry_config->temp_telem_buffer_size;
    pvt_telem_cfg->temp_dma_settings.buffer_stride = p_telemetry_config->temp_telem_stride_size;
    pvt_telem_cfg->volt_dma_settings.buffer_size = p_telemetry_config->volt_telem_buffer_size;
    pvt_telem_cfg->volt_dma_settings.buffer_stride = p_telemetry_config->volt_telem_stride_size;
}

static PLACED_CODE uint16_t power_init_find_sochot_safe_temp_hyst(uint16_t config_hyst, dts_coeff_t fused_coeff)
{
    uint16_t raw_hyst = TEMPTHRESHOLD2DOUT(config_hyst, fused_coeff);
    while ((raw_hyst < UINT16_MAX) && (power_hw_dts_pvt_raw_to_temp_dC(raw_hyst, fused_coeff) < config_hyst))
    {
        // basically, we want to increase the raw value until the temp calculated from the raw value is >= the input hysteresis temp
        // this will ensure that if we wait for polled temp < this value before clearing pmin, that we can ensure we went below the
        // hysteresis in PVT, which is what is required to rearm the PVT alarm
        ++raw_hyst;
    }
    return raw_hyst;
}

/**
 * @brief Updates pvt_config structure
 *
 * \b Description:
 *      Generates soc pvt config from p_knobs/fuse detail
 *
 * @param[in] p_runconfig - power runtime config
 * @param[inout] pvt_cfg - pvt config structure
 *
 * @return None
 *
 */
static PLACED_CODE void power_init_update_socpvt_cfg(const power_runconfig_t* p_runconfig, pvt_setting_config_t* pvt_cfg)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);
    FPFW_RUNTIME_ASSERT(pvt_cfg != NULL);

    const power_knobs_t* p_knobs = &p_runconfig->knobs;
    const power_fuse_data_t* fuses = &p_runconfig->fuses;

    // ensure match between config and HW
    static_assert(NUM_SOC_VM == SOC_PVT_TOTAL_CHANNELS_VM, "Mismatch between HW library and config on SOC VM count");
    static_assert(ARRAY_SIZE(p_knobs->soc_vm.thresholds) == SOC_PVT_TOTAL_CHANNELS_VM,
                  "Mismatch between HW library and config manager on SOC VM count");

    pvt_cfg->dts_channels_config.setting_count = SOC_PVT_TOTAL_CHANNELS_DTS;
    pvt_cfg->dts_channels_config.alarm_settings = (pvt_alarm_setting_config_t*)&s_pvt_dts_alarm_settings;
    for (int dts_idx = 0; dts_idx < SOC_PVT_TOTAL_CHANNELS_DTS; ++dts_idx)
    {
        pvt_alarm_setting_config_t alarm_setting = {0};
        alarm_setting.alarma_enable = p_knobs->soc_temp_hot_en;
        alarm_setting.alarmb_enable = p_knobs->soc_temp_thermtrip_en;
        alarm_setting.alarma_thresholds.hyst_threshold =
            power_init_find_sochot_safe_temp_hyst(p_knobs->soc_temp.hot.hyst_threshold, fuses->dts_coeff_soctop[dts_idx]);
        alarm_setting.alarma_thresholds.alarm_threshold =
            TEMPTHRESHOLD2DOUT(p_knobs->soc_temp.hot.alarm_threshold, fuses->dts_coeff_soctop[dts_idx]);
        alarm_setting.alarmb_thresholds.hyst_threshold =
            TEMPTHRESHOLD2DOUT(p_knobs->soc_temp.thermtrip.hyst_threshold, fuses->dts_coeff_soctop[dts_idx]);
        alarm_setting.alarmb_thresholds.alarm_threshold =
            TEMPTHRESHOLD2DOUT(p_knobs->soc_temp.thermtrip.alarm_threshold, fuses->dts_coeff_soctop[dts_idx]);
        s_pvt_dts_alarm_settings[dts_idx] = alarm_setting;
    }
    pvt_cfg->vm_channels_config.setting_count = SOC_PVT_TOTAL_CHANNELS_VM;
    pvt_cfg->vm_channels_config.alarm_settings = (pvt_alarm_setting_config_t*)&s_pvt_vm_alarm_settings;
    for (int vm_idx = 0; vm_idx < SOC_PVT_TOTAL_CHANNELS_VM; ++vm_idx)
    {
        const bool div_by_2 = ((p_runconfig->p_sconfig->soc_vm[vm_idx].flags & VM_FLAGS_DIV2) != 0);
        pvt_alarm_setting_config_t alarm_setting = {0};
        alarm_setting.alarma_enable = p_knobs->soc_vm_overvolt_en;
        alarm_setting.alarmb_enable = p_knobs->soc_vm_undervolt_en;
        alarm_setting.alarma_thresholds.hyst_threshold =
            VOLTTHRESHOLD2DOUT(p_knobs->soc_vm.thresholds[vm_idx].overvolt.hyst_threshold, div_by_2);
        alarm_setting.alarma_thresholds.alarm_threshold =
            VOLTTHRESHOLD2DOUT(p_knobs->soc_vm.thresholds[vm_idx].overvolt.alarm_threshold, div_by_2);
        alarm_setting.alarmb_thresholds.hyst_threshold =
            VOLTTHRESHOLD2DOUT(p_knobs->soc_vm.thresholds[vm_idx].undervolt.hyst_threshold, div_by_2);
        alarm_setting.alarmb_thresholds.alarm_threshold =
            VOLTTHRESHOLD2DOUT(p_knobs->soc_vm.thresholds[vm_idx].undervolt.alarm_threshold, div_by_2);
        s_pvt_vm_alarm_settings[vm_idx] = alarm_setting;

        FPFW_DBGPRINT_INFO("VM %d: overvolt hyst %d, alarm %d, undervolt hyst %d, alarm %d",
                           vm_idx,
                           s_pvt_vm_alarm_settings[vm_idx].alarma_thresholds.hyst_threshold,
                           s_pvt_vm_alarm_settings[vm_idx].alarma_thresholds.alarm_threshold,
                           s_pvt_vm_alarm_settings[vm_idx].alarmb_thresholds.hyst_threshold,
                           s_pvt_vm_alarm_settings[vm_idx].alarmb_thresholds.alarm_threshold);
    }
}

/**
 * @brief Resets PVT after a warm reset
 *
 * \b Description:
 *      This function to reset PVT ahead of reprogramming telemetry settings on a warm reset.
 *
 * @param[in] p_runconfig - power runtime config
 *
 * @return FWK_SUCCESS or FWK_E_STATE
 *
 */

PLACED_CODE void power_warm_init_core_reset_pvt(const power_runconfig_t* p_runconfig)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    const power_service_config_t* p_config = p_runconfig->p_sconfig;
    const unsigned int core_count = p_config->platform_die_core_count;

    FPFW_RUNTIME_ASSERT(p_config != NULL);

    // PVT needs to be initialized for all present core,even if not enabled
    corebits_t platform_cores = {0};
    switch (idsw_get_platform_sdv())
    {
    case PLATFORM_SVP_SIM:
        platform_cores = SVP_CORES;
        break;
    case PLATFORM_SVP_MIN_CONFIG_SIM:
        platform_cores = SVP_MIN_CORES;
        break;
    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        platform_cores = FPGA_PLATFORM_CORES;
        break;
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D_8C:
        platform_cores = ZEBU_CORES_8C;
        break;
    case PLATFORM_RVP_EVT_SILICON:
        platform_cores = PLATFORM_CORES;
        break;
    default:
        POWER_LOG_INFO("Unknown Platform, PVT not initialized for this platform");
        break;
    }

    for (unsigned int core = 0; core < core_count; ++core)
    {
        if (!corebits_is_bit_set(&platform_cores, core))
        {
            // skip cores not present in platform
            continue;
        }
        const uintptr_t cluster_pex_base_addr = (p_config->cluster_pex_base + (p_config->cluster_stride * core));

        /* One tile per two cores */
        if (core % CORES_PER_TILE == 0)
        {
            const uint32_t tile_num = core / CORES_PER_TILE;
            // Only even cores have PVT
            reset_tile_pvt_dts_vm(cluster_pex_base_addr);
            int return_value = tile_pvt_sda_reconfig(cluster_pex_base_addr, false);
            if (return_value != PVT_SUCCESS)
            {
                POWER_LOG_CRIT("tile_pvt_sda_reconfig for tile %d returned %d", (int)tile_num, return_value);
                BUG_CHECK(KNG_SC_TILE_PVT_RECONFIG, tile_num, return_value);
            }
        }
    }
}

PLACED_CODE void power_init_ws_core(const power_runconfig_t* p_runconfig, const power_telcfg_t* p_telemetry_config)
{
    FPFW_RUNTIME_ASSERT(p_telemetry_config != NULL);
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    const power_service_config_t* p_config = p_runconfig->p_sconfig;
    const unsigned int core_count = p_config->platform_die_core_count;

    FPFW_RUNTIME_ASSERT(p_config != NULL);

    /* Only big fpga, zebu, rtl sim would have DVFS, etc */
    if (!(p_config->platform_core_power_support))
    {
        POWER_ET_ERROR(POWER_ET_TYPE_PLATFORM_NOT_SUPPORTED_SKIPPING_DVFS_INIT, ET_NOPARAM);
        POWER_LOG_INFO("Skipping intialization of power HW");
        return;
    }

    power_warm_init_core_reset_pvt(p_runconfig);

    POWER_ET_STATUS(POWER_ET_TYPE_DVFS_REINIT);
    POWER_LOG_INFO("Reinitializing DVFS telemetry");

    // PVT needs to be initialized for all present core,even if not enabled
    corebits_t platform_cores = {0};
    switch (idsw_get_platform_sdv())
    {
    case PLATFORM_SVP_SIM:
        platform_cores = SVP_CORES;
        break;
    case PLATFORM_SVP_MIN_CONFIG_SIM:
        platform_cores = SVP_MIN_CORES;
        break;
    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        platform_cores = FPGA_PLATFORM_CORES;
        break;
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D_8C:
        platform_cores = ZEBU_CORES_8C;
        break;
    case PLATFORM_RVP_EVT_SILICON:
        platform_cores = PLATFORM_CORES;
        break;
    default:
        POWER_LOG_INFO("Unknown Platform, PVT not initialized for this platform");
        break;
    }

    for (unsigned int core = 0; core < core_count; ++core)
    {
        const uintptr_t cluster_pex_base_addr = (p_config->cluster_pex_base + (p_config->cluster_stride * core));
        const bool core_enabled = ift_is_enabled() || corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core);

        if (!corebits_is_bit_set(&platform_cores, core))
        {
            // skip cores not present in platform
            continue;
        }

        // PVT sensor needs to be initialized for all cores on platform, even if not enabled
        /* One tile per two cores */
        if (core % CORES_PER_TILE == 0)
        {
            const uint32_t tile_num = core / CORES_PER_TILE;
            tile_pvt_telem_setting_config_t tile_pvt_telem_settings = PVT_TILE_TELEM_DEFAULT_CONFIG;

            //! Update the sample itr cnt for both voltage and temperature for DMA to write every 4th sample
            //! to sensor ram Must be set for both cold start and warm start paths
            tile_pvt_telem_settings.temp_dma_settings.sampling_iteration_count = SAMPLING_CONFIG_DTS_ITER_CNT;
            tile_pvt_telem_settings.volt_dma_settings.sampling_iteration_count = SAMPLING_CONFIG_VM_ITER_CNT;

            // update tile pvt telemetry addreses
            power_init_update_tilepvt_telemetry_cfg(&tile_pvt_telem_settings, p_telemetry_config, tile_num);
            tile_pvt_dma_config(cluster_pex_base_addr, &tile_pvt_telem_settings);
        }

        // Update ODCM and DVFS telemetry for present and enabled cores
        if (core_enabled)
        {
            odcm_telem_config_t odcm_telem_cfg = ODCM_TELEM_DEFAULT_CONFIG;

            power_init_update_odcm_cfg_core(&odcm_telem_cfg, p_telemetry_config, core);
            odcm_telemetry_config(cluster_pex_base_addr, &odcm_telem_cfg);

            // update dvfs config, including core-specific init
            dvfs_telemetry_config(cluster_pex_base_addr,
                                  p_telemetry_config->pstate_telem_wr_address,
                                  p_telemetry_config->scp_msg_telem_wr_address);

            dvfs_set_plimit(cluster_pex_base_addr, MAX_PLIMIT, false);
        }
    }
}

PLACED_CODE void power_init_core(const power_runconfig_t* p_runconfig, const power_telcfg_t* p_telemetry_config)
{
    FPFW_RUNTIME_ASSERT(p_telemetry_config != NULL);
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    const power_service_config_t* p_config = p_runconfig->p_sconfig;
    const unsigned int core_count = p_config->platform_die_core_count;
    const power_knobs_t* p_knobs = &p_runconfig->knobs;

    FPFW_RUNTIME_ASSERT(p_config != NULL);

    dvfs_vft_t forced_pstate_vft = {0}; // temp vft to use for any cores with forced pstates
    int return_value = 0;

    /* Only big fpga, zebu, rtl sim would have DVFS, etc */
    if (!(p_config->platform_core_power_support))
    {
        POWER_ET_ERROR(POWER_ET_TYPE_PLATFORM_NOT_SUPPORTED_SKIPPING_DVFS_INIT, ET_NOPARAM);
        POWER_LOG_INFO("Skipping intialization of power HW");
        return;
    }

    dvfs_config_t dvfs_cfg = DVFS_DEFAULT_CONFIG;
    power_init_update_dvfs_cfg_common(p_runconfig, &dvfs_cfg, p_telemetry_config);

    /* Base ODCM config - Single ODCM MMA config for all instances */
    odcm_config_t odcm_cfg = ODCM_DEFAULT_CONFIG;
    power_init_update_odcm_cfg_common(&odcm_cfg, p_knobs);

    /* Base tile PVT config - Single config for all instances */
    pvt_setting_config_t tile_pvt_settings = PVT_TILE_SETTING_DEFAULT;
    if (idsw_get_platform_sdv() == PLATFORM_RVP_EVT_SILICON)
    {
        tile_pvt_settings.target_dts_clk_freq_khz = TILE_PVT_TARGET_DTS_FREQ_KHZ_POR;
        tile_pvt_settings.target_vm_clk_freq_khz = TILE_PVT_TARGET_VM_FREQ_KHZ_POR;
    }
    tile_pvt_telem_setting_config_t tile_pvt_telem_settings = PVT_TILE_TELEM_DEFAULT_CONFIG;

    //! Update the sample itr cnt for both voltage and temperature for DMA to write every 4th sample to sensor ram
    //! Figures based on the por dts/vm clk freq khz settings, limits unnecessary telemetry traffic
    //! DTS_ITER_CNT = 3, transmit every 4th sample (or every ~918uS)
    //! VM_ITER_CNT = 3, transmit every 4th sample (or every ~9.17mS, should be less than pvt loop interval or 10ms)
    tile_pvt_telem_settings.temp_dma_settings.sampling_iteration_count = SAMPLING_CONFIG_DTS_ITER_CNT;
    tile_pvt_telem_settings.volt_dma_settings.sampling_iteration_count = SAMPLING_CONFIG_VM_ITER_CNT;

    power_init_update_tilepvt_cfg(p_runconfig, &tile_pvt_settings);

    POWER_ET_STATUS(POWER_ET_TYPE_DVFS_INIT);
    POWER_LOG_INFO("Initializing DVFS");

    corebits_t platform_cores = {0};
    switch (idsw_get_platform_sdv())
    {
    case PLATFORM_SVP_SIM:
        platform_cores = SVP_CORES;
        break;
    case PLATFORM_SVP_MIN_CONFIG_SIM:
        platform_cores = SVP_MIN_CORES;
        break;
    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        platform_cores = FPGA_PLATFORM_CORES;
        break;
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D_8C:
        platform_cores = ZEBU_CORES_8C;
        break;
    case PLATFORM_EMU:
    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_2D:
        platform_cores = ZEBU_PLATFORM_CORES;
        break;
    case PLATFORM_RVP_EVT_SILICON:
        platform_cores = PLATFORM_CORES;
        break;
    default:
        POWER_LOG_INFO("Unknown Platform, PVT not initialized for this platform");
        break;
    }

    for (unsigned int core = 0; core < core_count; ++core)
    {
        const uintptr_t cluster_pex_base_addr = (p_config->cluster_pex_base + (p_config->cluster_stride * core));
        const bool core_enabled = ift_is_enabled() || corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core);

        if (!corebits_is_bit_set(&platform_cores, core))
        {
            // skip cores not present in platform
            continue;
        }

        // PVT sensor needs to be initialized for all cores on platform, even if not enabled
        /* One tile per two cores */
        if (core % CORES_PER_TILE == 0)
        {
            const uint32_t tile_num = core / CORES_PER_TILE;

            // update tile pvt DTS alarms
            power_init_update_tilepvt_tile_cfg(p_runconfig, &tile_pvt_settings, tile_num);
            // update tile pvt telemetry addreses
            power_init_update_tilepvt_telemetry_cfg(&tile_pvt_telem_settings, p_telemetry_config, tile_num);

            return_value = tile_pvt_init(tile_num, cluster_pex_base_addr, &tile_pvt_telem_settings, &tile_pvt_settings);
            if (return_value != PVT_SUCCESS)
            {
                POWER_LOG_CRIT("tile_pvt_init for tile %d returned %d", (int)tile_num, return_value);
                BUG_CHECK(KNG_SC_TILE_PVT_INIT, tile_num, return_value);
            }
        }

        // Initialize ODCM and DVFS for present and enabled cores
        if (core_enabled)
        {

            core_pll_mask_error_sr(p_runconfig, core);

            power_init_update_odcm_cfg_core(&odcm_cfg.telem_cfg, p_telemetry_config, core);
            return_value = odcm_init(cluster_pex_base_addr, &odcm_cfg);
            if (return_value != ODCM_SUCCESS)
            {
                POWER_LOG_CRIT("odcm_init for core %d returned %d", core, return_value);
                BUG_CHECK(KNG_SC_CORE_ODCM_INIT, core, return_value);
            }

            // update dvfs config, including core-specific init
            power_init_update_dvfs_cfg_core(p_runconfig, &dvfs_cfg, core);

            // reconfigure PLLs and VMAT if we're forcing a pstate
            if (p_knobs->force_pstate < NUM_PSTATES)
            {
                // determine core's curve minimum plimit/pstate
                const unsigned int assigned_vft = p_runconfig->derived.assigned_vft[core];
                const uint8_t curve_min_plimit = p_runconfig->derived.vfts[assigned_vft].min_plimit;
                uint8_t forced_pstate = p_knobs->force_pstate;

                // clip forced pstate if less than supported by curve
                if (forced_pstate < curve_min_plimit)
                {
                    POWER_ET_ERROR(POWER_ET_TYPE_FORCE_PSTATE_CLIPPED,
                                   POWER_ET_ENCODE_DETAIL_CORE(curve_min_plimit, core));
                    POWER_LOG_ERR("force_pstate clipped for core %u: requested %u, core/curve minimum: %u",
                                  core,
                                  forced_pstate,
                                  curve_min_plimit);
                    forced_pstate = curve_min_plimit;
                }

                // TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491054/
                // for ITD, need to make sure ITD is disabled for force-pstate (would need highest voltage for
                // any pstate across the curves) or need to make sure we have 4 updated curves using this api
                setup_forced_pstate(cluster_pex_base_addr, &dvfs_cfg, &forced_pstate_vft, core, forced_pstate);
            }
        }
        else
        {
            power_init_update_dvfs_cfg_disable(&dvfs_cfg);
        }

        /**
         * Set the software boot mode if IFT is enabled
         * This turns on the LDO and PLL without waiting for a PPU handshake
         */
        if (ift_is_enabled())
        {
            dvfs_cfg.init_cfg.sw_boot_mode = true;
        }

        return_value = dvfs_init(cluster_pex_base_addr, &dvfs_cfg);
        if (return_value != DVFS_SUCCESS)
        {
            POWER_LOG_CRIT("dvfs_init for core %d returned %d", core, return_value);
            BUG_CHECK(KNG_SC_CORE_DVFS_INIT, core, return_value);
        }
    }
    // split the end of init out from above to parallelize the FLL calibrations
    for (unsigned int core = 0; core < core_count; ++core)
    {
        const uintptr_t cluster_pex_base_addr = (p_config->cluster_pex_base + (p_config->cluster_stride * core));
        const bool core_enabled = ift_is_enabled() || corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core);

        if (!corebits_is_bit_set(p_config->platform_cores_in_die, core))
        {
            // skip cores not present in platform
            continue;
        }

        if (core_enabled)
        {
            // wait for FLL calibration done
            uint32_t fll_cal_timeout = IS_PLATFORM_FPGA() ? FLL_CAL_TIMEOUT_FPGA_US : FLL_CAL_TIMEOUT_US;
            return_value = wait_for_FLLCalDone(cluster_pex_base_addr, fll_cal_timeout);
            if (return_value != DVFS_SUCCESS)
            {
                POWER_LOG_CRIT("wait_for_FLLCalDone on core %d returned %d", core, return_value);
                BUG_CHECK(KNG_SC_CORE_WAIT_FLL_CAL, core, return_value);
            }

            // make desired CPPC request to match expected nominal perf and base perf
            dvfs_ns_set_cppc_desired_perf(cluster_pex_base_addr,
                                          dvfs_get_cppc_from_pstate(p_runconfig->derived.pnominal),
                                          dvfs_get_cppc_from_pstate(p_runconfig->derived.pnominal),
                                          0,
                                          0);
        }
    }
}

PLACED_CODE void power_init_soc(const power_runconfig_t* p_runconfig)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    const power_service_config_t* p_config = p_runconfig->p_sconfig;
    FPFW_RUNTIME_ASSERT(p_config != NULL);

    /* Only big fpga, zebu, rtl sim would have soc pvt */
    if (!(p_config->platform_soc_power_support))
    {
        POWER_ET_ERROR(POWER_ET_TYPE_PLATFORM_NOT_SUPPORTED_SKIPPING_SOC_HW_INIT, ET_NOPARAM);
        POWER_LOG_INFO("Skipping intialization of soc power HW");
        return;
    }

    pvt_setting_config_t soc_pvt_settings = PVT_SOC_SETTING_DEFAULT;
    if (idsw_get_platform_sdv() == PLATFORM_RVP_EVT_SILICON)
    {
        soc_pvt_settings.target_dts_clk_freq_khz = SOC_PVT_TARGET_DTS_FREQ_KHZ_POR;
        soc_pvt_settings.target_vm_clk_freq_khz = SOC_PVT_TARGET_VM_FREQ_KHZ_POR;
    }
    power_init_update_socpvt_cfg(p_runconfig, &soc_pvt_settings);

    int status = soc_pvt_init(p_config->soc_pvt_base, &soc_pvt_settings);
    if (status != PVT_SUCCESS)
    {
        POWER_LOG_CRIT("soc_pvt_init returned %d", status);
        BUG_CHECK(KNG_SC_SOC_PVT_INIT, 0, status);
    }
}

void power_set_plimit(const power_runconfig_t* p_runconfig, unsigned int core, dvfs_plimit plimit)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    const power_service_config_t* p_config = p_runconfig->p_sconfig;
    FPFW_RUNTIME_ASSERT(p_config != NULL);

    const uintptr_t cluster_pex_base_addr = (p_config->cluster_pex_base + (p_config->cluster_stride * core));
    dvfs_config_plimit(cluster_pex_base_addr, plimit);
}

uint32_t power_hw_get_adclk_count(const power_runconfig_t* p_runconfig, unsigned int core)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    const power_service_config_t* p_config = p_runconfig->p_sconfig;
    FPFW_RUNTIME_ASSERT(p_config != NULL);

    const uintptr_t cluster_pex_base_addr = (p_config->cluster_pex_base + (p_config->cluster_stride * core));
    const bool core_enabled = ift_is_enabled() || corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core);

    uint32_t droop_count = 0;

    if (core_enabled)
    {
        dvfs_get_adclk_droop_count(cluster_pex_base_addr, &droop_count);
    }
    return droop_count;
}

bool power_hw_uses_pvt_model()
{
    return false;
}

bool power_hw_full_init_allowed()
{
    // only allow full init if cold boot
    return !system_info_is_warm_start();
}

uint8_t power_hw_pstate_from_freq(uint16_t freq_MHz)
{
    for (unsigned pidx = 0; pidx < NUM_PSTATES; ++pidx)
    {
        if (dvfs_get_freq_from_plimit(pidx) <= freq_MHz)
        {
            return pidx;
        }
    }
    return MAX_PLIMIT;
}

void core_pll_mask_error_sr(const power_runconfig_t* p_runconfig, int core)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);

    const power_service_config_t* p_config = p_runconfig->p_sconfig;
    FPFW_RUNTIME_ASSERT(p_config != NULL);

    const power_knobs_t* knobs = &p_runconfig->knobs;
    uintptr_t cluster_pex_base_addr = (p_config->cluster_pex_base + (p_config->cluster_stride * core));
    uintptr_t fgpll_addr = cluster_pex_base_addr + PEX_CORE_PLL_ADDRESS;
    vptr_fgpll_reg pll_regs = (vptr_fgpll_reg)fgpll_addr;

    fgpll_pll_error_mask_cr pll_error_mask = {.as_uint32 = 0};

    pll_error_mask.plllockrawmask = knobs->plllock_cfg.mask_plllockraw ? 0x1 : 0x0;
    pll_error_mask.freqchangetimeoutmask = knobs->plllock_cfg.mask_freqchangetimeout ? 0x1 : 0x0;
    pll_error_mask.fllcaltimeoutmask = knobs->plllock_cfg.mask_fllcaltimeout ? 0x1 : 0x0;
    pll_error_mask.locktimeoutmask = knobs->plllock_cfg.mask_locktimeout ? 0x1 : 0x0;
    pll_error_mask.freqerrormask = knobs->plllock_cfg.mask_freqerror ? 0x1 : 0x0;
    // if any bits were set, set the override
    pll_error_mask.override = (pll_error_mask.as_uint32 != 0) ? 0x1 : 0x0;

    pll_regs->pll_error_mask_cr.as_uint32 = pll_error_mask.as_uint32;
}

void power_hw_clear_force_pmin(power_pmin_type_t type)
{
    power_runconfig_t* runconfig = power_runconfig_get();
    vptr_scp_exp_csr_reg scp_exp_csr_regs_base = (vptr_scp_exp_csr_reg)runconfig->p_sconfig->scp_exp_csr_base;

    switch (type)
    {
    case PM_PMIN_ALL:
        scp_exp_csr_regs_base->force_pmin_reg.as_uint32 = 0;
        break;

    case PM_LOCKUP_UE_RR:
        scp_exp_csr_regs_base->force_pmin_reg.lockup_ue_rr = 0;
        break;

    case PM_FW_PMIN_CONTROL:
        scp_exp_csr_regs_base->force_pmin_reg.fw_pmin_control = 0;
        break;

    default:
        break;
    }
}

void power_hw_force_pmin(power_pmin_type_t type)
{
    power_runconfig_t* config = power_runconfig_get();
    // expect that there's never a reason to set all the PMIN bits
    FPFW_RUNTIME_ASSERT(type != PM_PMIN_ALL);

    vptr_scp_exp_csr_reg scp_exp_csr_regs_base = (vptr_scp_exp_csr_reg)config->p_sconfig->scp_exp_csr_base;

    switch (type)
    {
    case PM_LOCKUP_UE_RR:
        scp_exp_csr_regs_base->force_pmin_reg.lockup_ue_rr = 1;

        break;
    case PM_FW_PMIN_CONTROL:
        scp_exp_csr_regs_base->force_pmin_reg.fw_pmin_control = 1;
        break;

    case PM_PMIN_ALL:
    default:
        break;
    }
}

void power_hw_set_thermal_io_state(power_io_thermal_trigger_type_t type, bool state)
{
    power_runconfig_t* config = power_runconfig_get();
    FPFW_RUNTIME_ASSERT(type != MAX_IO_THERMAL_TRIGGER_TYPE);

    vptr_scp_exp_csr_reg scp_exp_csr_regs_base = (vptr_scp_exp_csr_reg)config->p_sconfig->scp_exp_csr_base;

    switch (type)
    {
    case SOC_HOT:
        scp_exp_csr_regs_base->thermal_io_reg.soc_hot = state ? 1 : 0;
        break;
    case MEM_HOT:
        scp_exp_csr_regs_base->thermal_io_reg.memory_hot = state ? 1 : 0;
        break;
    case THERM_TRIP:
        scp_exp_csr_regs_base->thermal_io_reg.thermal_trip = state ? 1 : 0;
        break;
    default:
        break;
    }
}

// when the HW boots, this function can be used to capture initial state that would be delivered via update messages at runtime;
// this is mainly for warmboot scenario where AP core is already running, but useful at cold boot, also
void power_hw_capture_cppc_state(power_hw_update_cb_t p_update_cb)
{
    power_runconfig_t* p_runconfig = power_runconfig_get();

    // confirm the parameter isn't null
    FPFW_RUNTIME_ASSERT(p_update_cb != NULL);

    // iterate over all cores
    for (unsigned int core = 0; core < NUM_AP_CORES_PER_DIE; ++core)
    {
        // skip cores not present or disabled
        if (!ift_is_enabled() && !corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core))
        {
            continue;
        }
        // get the base address for the core
        uintptr_t cluster_pex_base_addr =
            (p_runconfig->p_sconfig->cluster_pex_base + (p_runconfig->p_sconfig->cluster_stride * core));

        // get the current CPPC state
        uint8_t cppc_desired_perf;
        uint8_t cppc_base_perf;
        uint8_t throttle_pri;
        uint8_t boost_pri;
        int status =
            dvfs_ns_get_cppc_desired_perf(cluster_pex_base_addr, &cppc_desired_perf, &cppc_base_perf, &throttle_pri, &boost_pri);
        // only real failure should be invalid pointers
        BUG_ASSERT_PARAM(status == DVFS_SUCCESS, status, 0);
        // call update callback with detail from this core's CPPC detail
        p_update_cb(core, dvfs_get_pstate_from_cppc(cppc_desired_perf), dvfs_get_pstate_from_cppc(cppc_base_perf), throttle_pri, boost_pri);
    }
}

void parse_dvfs_throttle_status(uintptr_t reg_addr)
{
    uint32_t value = *((volatile uint32_t*)reg_addr);
    printf("DVFS_THROT_SR register address: 0x%lx, value: 0x%" PRIx32 "\n", (unsigned long)reg_addr, value);

    // Individual bit descriptions
    const char* bit_descriptions[] = {
        "adclkThrottle_Q",      // bit 0
        "endAdclkThrottle_Q",   // bit 1
        "startAdclkThrottle_Q", // bit 2
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,                  // bits 3-7 are a field
        "currThrottle_Q",      // bit 8
        "endCurrThrottle_Q",   // bit 9
        "startCurrThrottle_Q", // bit 10
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,                  // bits 11-15 are a field
        "tempThrottle_Q",      // bit 16
        "endTempThrottle_Q",   // bit 17
        "startTempThrottle_Q", // bit 18
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,                 // bits 19-23 are a field
        "adClkThrottle",      // bit 24
        "ODCM_cteAlarm",      // bit 25
        "ODCM_cteAmt",        // bit 26
        "DTS_tteAlarm",       // bit 27
        "SYS_forcePmin_syncQ" // bit 28
        // bits 29-31 are reserved
    };

    for (int i = 0; i <= 28; ++i)
    {
        if (bit_descriptions[i] && (value & (1 << i)))
        {
            printf("%s is SET\n", bit_descriptions[i]);
        }
    }

    // Bit fields
    uint8_t adclkThrottleTargetPn_Q = (value >> 3) & 0x1F;
    if (adclkThrottleTargetPn_Q)
    {
        printf("adclkThrottleTargetPn_Q[4:0] = %u\n", adclkThrottleTargetPn_Q);
    }

    uint8_t currThrottleTargetPn_Q = (value >> 11) & 0x1F;
    if (currThrottleTargetPn_Q)
    {
        printf("currThrottleTargetPn_Q[4:0] = %u\n", currThrottleTargetPn_Q);
    }

    uint8_t tempThrottleTargetPn_Q = (value >> 19) & 0x1F;
    if (tempThrottleTargetPn_Q)
    {
        printf("tempThrottleTargetPn_Q[4:0] = %u\n", tempThrottleTargetPn_Q);
    }

    uint8_t reserved = (value >> 29) & 0x7;
    if (reserved != 0)
    {
        printf("Reserved bits [31:29] = %u (Expected 000)\n", reserved);
    }
}

bool all_requests_completed(avs_pwr_request_context_t* pwr_avs_request, uint8_t max_avs_bus)
{
    BUG_ASSERT(max_avs_bus <= MAX_AVS_INST);
    for (int i = 0; i < max_avs_bus; i++)
    {
        if (pwr_avs_request[i].in_use)
        {
            return false;
        }
    }
    return true;
}

bool no_errors(avs_pwr_request_context_t* pwr_avs_request, uint8_t max_avs_bus)
{
    BUG_ASSERT(max_avs_bus <= MAX_AVS_INST);
    for (int i = 0; i < max_avs_bus; i++)
    {
        if (pwr_avs_request[i].error)
        {
            return false;
        }
    }
    return true;
}

void reset_errors(avs_pwr_request_context_t* pwr_avs_request, uint8_t max_avs_bus)
{
    BUG_ASSERT(max_avs_bus <= MAX_AVS_INST);
    for (int i = 0; i < max_avs_bus; i++)
    {
        pwr_avs_request[i].error = false;
    }
}
