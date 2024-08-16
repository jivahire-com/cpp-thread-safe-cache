//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_warmstart.c
 * Implementation of power warm start.
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>        // for FPFW_RUNTIME_ASSERT
#include <bug_check.h>         // for BUG_CHECK
#include <corebits.h>          // for corebits_t
#include <dvfs.h>              // for dvfs_mvolt_from_ldo_dac
#include <kng_error.h>         // for KNG_SC_FUSE_LDO_MVOLT_CONV, KNG_SC_WARMSTART_DATA_CORRUPT..
#include <power_i.h>           // for power_runconfig_t
#include <power_warmstart_i.h> // for power_ws_fuse_t, ppower_ws_fuse_t
#include <warm_start.h>        // for ws_data_get, ws_data_put, ws_data_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @brief Generate warm start data from the current power run configuration.
 *
 * @param runconfig [in]
 *      The current power run configuration.
 * @param ws_fuse_data [out]
 *      The warm start data to be generated.
 */
static void power_ws_generate_fuse_data(const power_runconfig_t* runconfig, power_ws_fuse_t* ws_fuse_data)
{
    FPFW_RUNTIME_ASSERT(runconfig != NULL);
    FPFW_RUNTIME_ASSERT(ws_fuse_data != NULL);

    ws_fuse_data->version = POWER_WARMSTART_CURRENT_VER;
    ws_fuse_data->valid_cores = runconfig->fuses.valid_cores;
    ws_fuse_data->mpmm.enable = runconfig->knobs.mpmm.enable;
    ws_fuse_data->mpmm.gear = runconfig->knobs.mpmm.gear;

    // store the freq/voltage points configured at boot to use in control loop after a warm reset
    for (unsigned vf_idx = 0; vf_idx < WS_VFT_COUNT; ++vf_idx)
    {
        ws_fuse_data->vfts[vf_idx].min_plimit = runconfig->derived.vfts[vf_idx].min_plimit;
        ws_fuse_data->vfts[vf_idx].cores = runconfig->derived.vfts[vf_idx].assigned_cores;

        for (unsigned pstate_idx = runconfig->derived.vfts[vf_idx].min_plimit; pstate_idx < NUM_PSTATES; ++pstate_idx)
        {
            // Store the ldo dac in values for each curve
            for (unsigned curve_idx = 0; curve_idx < VFT_CURVE_COUNT_PER_CURVESET; ++curve_idx)
            {
                ws_fuse_data->vfts[vf_idx].vf[pstate_idx].ldo_dac_in[curve_idx] =
                    runconfig->dvfs_vft.curveset[vf_idx].curve[curve_idx].vmat_info[0].ldo_dac_in[pstate_idx];
            }

            // Store the freq/voltage points for each pstate
            ws_fuse_data->vfts[vf_idx].vf[pstate_idx].voltage_mv =
                runconfig->derived.vfts[vf_idx].vf[pstate_idx].voltage_mv;
            ws_fuse_data->vfts[vf_idx].vf[pstate_idx].freq_Mhz =
                runconfig->derived.vfts[vf_idx].vf[pstate_idx].freq_Mhz;
        }
    }
}

/**
 * @brief Store the original warm start data in the runconfig.
 *
 * @param runconfig [in]
 *      The current power run configuration.
 */
static void ws_store_original_fuse_data(power_runconfig_t* runconfig)
{
    FPFW_RUNTIME_ASSERT(runconfig != NULL);

    static power_ws_fuse_t original_ws_fuse_data = {0};

    // Generate WS data from this boot (prior to being replaced)
    power_ws_generate_fuse_data(runconfig, &original_ws_fuse_data);

    // Store in runconfig so we can use it later
    BUG_ASSERT(runconfig->original_warmstart == NULL);
    runconfig->original_warmstart = (void*)&original_ws_fuse_data;
}

/**
 * @brief Recover the runconfig from the stored warm start data structure.
 *
 * @param runconfig [inout]
 *      Power runconfig structure to recover from warm start data.
 */
void power_ws_recover_fuse_init(power_runconfig_t* runconfig)
{
    FPFW_RUNTIME_ASSERT(runconfig != NULL);

    uint32_t ws_size = 0;
    ppower_ws_fuse_t ws_fuse_data = (ppower_ws_fuse_t)ws_data_get(WARM_START_ID_POWER_FUSE, &ws_size);

    // we need warmstart data to understand cold boot config and it's not present, do not continue
    if (ws_fuse_data == NULL)
    {
        BUG_CHECK(KNG_SC_WARMSTART_DATA_MISSING, 0, 0);
    }
    // data needs to at least be large enough to contain a version information
    if (ws_size < sizeof(ws_fuse_data->version))
    {
        BUG_CHECK(KNG_SC_WARMSTART_DATA_VERSION, 0, ws_size);
    }

    // Save off version of warmstart data before restore
    ws_store_original_fuse_data(runconfig);

    // If warm start data version changes, this switch statement will need to handle converting from stored
    // data to whatever suits the current version of the code.
    switch (ws_fuse_data->version)
    {
    case POWER_WARMSTART_VER1:
        if (ws_size != sizeof(power_ws_fuse_t))
        {
            BUG_CHECK(KNG_SC_WARMSTART_DATA_CORRUPT, ws_fuse_data->version, ws_size);
        }
        runconfig->fuses.valid_cores = ws_fuse_data->valid_cores;
        runconfig->knobs.mpmm.enable = ws_fuse_data->mpmm.enable;
        runconfig->knobs.mpmm.gear = ws_fuse_data->mpmm.gear;
        // Restore the freq/voltage points configured at boot to use in control loop after a warm reset
        for (unsigned vf_idx = 0; vf_idx < WS_VFT_COUNT; ++vf_idx)
        {
            runconfig->derived.vfts[vf_idx].min_plimit = ws_fuse_data->vfts[vf_idx].min_plimit;
            runconfig->derived.vfts[vf_idx].assigned_cores = ws_fuse_data->vfts[vf_idx].cores;

            for (unsigned pstate_idx = runconfig->derived.vfts[vf_idx].min_plimit; pstate_idx < NUM_PSTATES; ++pstate_idx)
            {
                // We also saved mv points from curve, but assumption would be that we can recalculate those if the calculation changes
                uint16_t max_volatage_mv = 0;

                for (unsigned crv_idx = 0; crv_idx < VFT_CURVE_COUNT_PER_CURVESET; ++crv_idx)
                {
                    uint16_t voltage_mv;
                    int status = dvfs_mvolt_from_ldo_dac(ws_fuse_data->vfts[vf_idx].vf[pstate_idx].ldo_dac_in[crv_idx],
                                                         &runconfig->fuses.ldodac_to_volt,
                                                         &voltage_mv);
                    if (DVFS_SUCCESS != status)
                    {
                        BUG_CHECK(KNG_SC_FUSE_LDO_MVOLT_CONV, (pstate_idx << 16) | (crv_idx << 8) | vf_idx, status);
                    }
                    max_volatage_mv = MAX(max_volatage_mv, voltage_mv);
                }

                runconfig->derived.vfts[vf_idx].vf[pstate_idx].voltage_mv = max_volatage_mv;
                runconfig->derived.vfts[vf_idx].vf[pstate_idx].freq_Mhz =
                    ws_fuse_data->vfts[vf_idx].vf[pstate_idx].freq_Mhz;
            }
        }
        break;
    default:
        POWER_LOG_CRIT(MODULE_NAME "Unknown warm start data version");
        BUG_CHECK(KNG_SC_WARMSTART_DATA_UNKNOWN, ws_fuse_data->version, 0);
    }
}

/**
 * @brief Initialize the warm start data and store in the warm start data structure.
 *
 * @param runconfig [in]
 *      Power runconfig structure to save.
 */
void power_ws_save_fuse_init(const power_runconfig_t* runconfig)
{
    FPFW_RUNTIME_ASSERT(runconfig != NULL);

    power_ws_fuse_t ws_fuse_data = {0};
    power_ws_generate_fuse_data(runconfig, &ws_fuse_data);

    // Store warm start fused-based configuration data.
    void* ws_data = ws_data_put(WARM_START_ID_POWER_FUSE, &ws_fuse_data, sizeof(ws_fuse_data));
    BUG_ASSERT(ws_data != NULL);
}