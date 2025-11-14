//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_fuses.c
 * Implementation of power fuse reads
 */

/*------------- Includes -----------------*/
#include "power_runconfig.h"   // for power_vcpu_interp_t, power_fuse_data_t
#include "power_runconfig_i.h" // for power_runconfig_t, power_runconfig_get
#include "pvt_struct.h"        // for TILE_PVT_NUM_CHANNELS_DTS

#include <FpFwAssert.h>  // for FPFW_RUNTIME_ASSERT
#include <bug_check.h>   // for BUG_CHECK
#include <core_info.h>   // for core_info_get_enable_core
#include <corebits.h>    // for corebits_set_bit, corebits_clear_bit
#include <dvfs_struct.h> // for dvfs_core_memasst_entry_t, dvfs_core_...
#include <fuse.h>        //fuse_read
#include <idsw_kng.h>
#include <kingsgate_fuse_defines.h> // for VF_CORE_ANCHOR_SEL_WIDTH, CORE_CDYN_0...
#include <kng_soc_constants.h>      // for NUM_AP_CORES_PER_DIE
#include <power_events.h>           // for POWER_ET_WARN, POWER_ET_ENCODE_FREQ_C...
#include <power_i.h>                // for DIMOF, BUG_ASSERT, BUG_CHECK, MODULE_...
#include <silibs_common.h>          // for ARRAY_SIZE
#include <stdbool.h>                // for bool
#include <stddef.h>                 // for NULL
#include <stdint.h>                 // for uint32_t, uint8_t, int32_t, uint64_t
#include <string.h>                 // for memset
/*-- Symbolic Constant Macros (defines) --*/

// defaults for gradient/offset equation for platforms where fuse isn't available
#define DVFS_LDODAC_TO_VOLT_SLOPE  2000
#define DVFS_LDODAC_TO_VOLT_OFFSET 2000
// defaults for dts k/y values for platforms where fuse isn't available
#define PVT_DTS_DEFAULT_K_VAL 286
#define PVT_DTS_DEFAULT_Y_VAL 649
// default for TDP power
#define TDP_DEFAULT_POWER_A 400

#define VF_CURVE_TEMP_RANGE_OFFSET 0x7F // offset for VF curve temp ranges in fuses
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

// core disables
static const uint32_t core_disables_fuse_offsets[] = {CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_31_0_BIT_OFFSET,
                                                      CORE_DEFECT_IFT_MASK_CORE_DEFECT_IFT_31_0_BIT_OFFSET,
                                                      CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_63_32_BIT_OFFSET,
                                                      CORE_DEFECT_IFT_MASK_CORE_DEFECT_IFT_63_32_BIT_OFFSET,
                                                      CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_67_64_BIT_OFFSET,
                                                      CORE_DEFECT_IFT_MASK_CORE_DEFECT_IFT_67_64_BIT_OFFSET};

static const uint32_t core_disables_fuse_widths[] = {CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_31_0_WIDTH,
                                                     CORE_DEFECT_IFT_MASK_CORE_DEFECT_IFT_31_0_WIDTH,
                                                     CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_63_32_WIDTH,
                                                     CORE_DEFECT_IFT_MASK_CORE_DEFECT_IFT_63_32_WIDTH,
                                                     CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_67_64_WIDTH,
                                                     CORE_DEFECT_IFT_MASK_CORE_DEFECT_IFT_67_64_WIDTH};

static const uint32_t curve_temp_ranges_offsets[] = {VF_CURVE_TEMP_RANGES_TEMP_THRESHOLD_T1_BIT_OFFSET,
                                                     VF_CURVE_TEMP_RANGES_TEMP_THRESHOLD_T2_BIT_OFFSET,
                                                     VF_CURVE_TEMP_RANGES_TEMP_THRESHOLD_T3_BIT_OFFSET};

static const uint32_t curve_temp_ranges_widths[] = {VF_CURVE_TEMP_RANGES_TEMP_THRESHOLD_T1_WIDTH,
                                                    VF_CURVE_TEMP_RANGES_TEMP_THRESHOLD_T2_WIDTH,
                                                    VF_CURVE_TEMP_RANGES_TEMP_THRESHOLD_T3_WIDTH};

// core memasst
#define COREMEMASST(number, field, type) DVFS_CORE_MEM_ASST_TRIMS_##number##_BOUND_##field##_##type
#define COREMEMASST_ALL(f, t) \
    COREMEMASST(0, f, t), COREMEMASST(1, f, t), COREMEMASST(2, f, t), COREMEMASST(3, f, t), COREMEMASST(4, f, t),
#define COREMEMASST_OFFSETS(field) COREMEMASST_ALL(field, BIT_OFFSET)
#define COREMEMASST_WIDTHS(field)  COREMEMASST_ALL(field, WIDTH)

static const uint32_t core_memasst_tpemab_offsets[] = {COREMEMASST_OFFSETS(TP_EMAB)};
static const uint32_t core_memasst_tpemab_widths[] = {COREMEMASST_WIDTHS(TP_EMAB)};
static const uint32_t core_memasst_tpemaa_offsets[] = {COREMEMASST_OFFSETS(TP_EMAA)};
static const uint32_t core_memasst_tpemaa_widths[] = {COREMEMASST_WIDTHS(TP_EMAA)};
static const uint32_t core_memasst_hshcema_offsets[] = {COREMEMASST_OFFSETS(HSHC_EMA)};
static const uint32_t core_memasst_hshcema_widths[] = {COREMEMASST_WIDTHS(HSHC_EMA)};
static const uint32_t core_memasst_hshcrawlm_offsets[] = {COREMEMASST_OFFSETS(HSHC_RAWLM)};
static const uint32_t core_memasst_hshcrawlm_widths[] = {COREMEMASST_WIDTHS(HSHC_RAWLM)};
static const uint32_t core_memasst_hdema_offsets[] = {COREMEMASST_OFFSETS(HD_EMA)};
static const uint32_t core_memasst_hdema_widths[] = {COREMEMASST_WIDTHS(HD_EMA)};
static const uint32_t core_memasst_hdrawlm_offsets[] = {COREMEMASST_OFFSETS(HD_RAWLM)};
static const uint32_t core_memasst_hdrawlm_widths[] = {COREMEMASST_WIDTHS(HD_RAWLM)};
static const uint32_t core_memasst_hd_emaw_offsets[] = {COREMEMASST_OFFSETS(HD_EMAW)};
static const uint32_t core_memasst_hd_emaw_widths[] = {COREMEMASST_WIDTHS(HD_EMAW)};
static const uint32_t core_memasst_ldodacin_offsets[] = {COREMEMASST_OFFSETS(LDODACIN)};
static const uint32_t core_memasst_ldodacin_widths[] = {COREMEMASST_WIDTHS(LDODACIN)};
static const uint32_t core_memasst_valid_boundary_offsets[] = {COREMEMASST_OFFSETS(VALID_BOUNDARY)};
static const uint32_t core_memasst_valid_boundary_widths[] = {COREMEMASST_WIDTHS(VALID_BOUNDARY)};

// helpers for VF fuse names
#define VF(curve_number, temp, field, index, type) VF##curve_number##_TEMP##temp##_##field##_##index##_##type
#define VFT(n, f, i, t)                            VF(n, 0, f, i, t), VF(n, 1, f, i, t), VF(n, 2, f, i, t), VF(n, 3, f, i, t)
#define VF_CURVE(n, f, t)                                                                                 \
    VFT(n, f, 6, t), VFT(n, f, 5, t), VFT(n, f, 4, t), VFT(n, f, 3, t), VFT(n, f, 2, t), VFT(n, f, 1, t), \
        VFT(n, f, 0, t)
#define VF_CURVES(f, t)                                                                                      \
    {VF_CURVE(0, f, t)}, {VF_CURVE(1, f, t)}, {VF_CURVE(2, f, t)}, {VF_CURVE(3, f, t)}, {VF_CURVE(4, f, t)}, \
        {VF_CURVE(5, f, t)}, {VF_CURVE(6, f, t)},

// arrays for VF fuse offsets and widths
static const uint32_t core_vft_fuse_freq_offsets[][VFT_CURVE_COUNT_PER_CURVESET * VFT_CURVESET_COUNT] = {
    VF_CURVES(FREQ, BIT_OFFSET)};
static const uint32_t core_vft_fuse_ldodac_offsets[][VFT_CURVE_COUNT_PER_CURVESET * VFT_CURVESET_COUNT] = {
    VF_CURVES(LDO_DAC_CODE, BIT_OFFSET)};
static const uint32_t core_vft_fuse_freq_widths[][VFT_CURVE_COUNT_PER_CURVESET * VFT_CURVESET_COUNT] = {
    VF_CURVES(FREQ, WIDTH)};
static const uint32_t core_vft_fuse_ldodac_widths[][VFT_CURVE_COUNT_PER_CURVESET * VFT_CURVESET_COUNT] = {
    VF_CURVES(LDO_DAC_CODE, WIDTH)};

// helpers for VPU_LEAKAGE offsets and widths
#define PFUSE(fuse, index, field, type) fuse##_##index##_##field##_##type
#define VL(index, field, type)          PFUSE(VCPU_LEAKAGE, index, field, type)
#define VL_ALL(f, t)                    VL(0, f, t), VL(1, f, t), VL(2, f, t), VL(3, f, t), VL(4, f, t), VL(5, f, t), VL(6, f, t)
#define VL_OFFSETS(f)                   VL_ALL(f, BIT_OFFSET)
#define VL_WIDTHS(f)                    VL_ALL(f, WIDTH)

// arrays for VCPU_LEAKAGE offsets and widths
static const uint32_t vcpu_leakage_temp_offsets[] = {VL_OFFSETS(TEMP)};
static const uint32_t vcpu_leakage_current_offsets[] = {VL_OFFSETS(CURRENT)};
static const uint32_t vcpu_leakage_ldo_offsets[] = {VL_OFFSETS(LDO)};
static const uint32_t vcpu_leakage_vcpu_offsets[] = {VL_OFFSETS(VCPU)};
static const uint32_t vcpu_leakage_temp_widths[] = {VL_WIDTHS(TEMP)};
static const uint32_t vcpu_leakage_current_widths[] = {VL_WIDTHS(CURRENT)};
static const uint32_t vcpu_leakage_ldo_widths[] = {VL_WIDTHS(LDO)};
static const uint32_t vcpu_leakage_vcpu_widths[] = {VL_WIDTHS(VCPU)};

// helpers for VCPU_LDO_DYN
#define VLD(index, field, type) PFUSE(VCPU_LDO_DYN, index, field, type)
#define VLD_ALL(f, t)           VLD(0, f, t), VLD(1, f, t),
#define VLD_OFFSETS(f)          VLD_ALL(f, BIT_OFFSET)
#define VLD_WIDTHS(f)           VLD_ALL(f, WIDTH)

// arrays for VCPU_LDO_DYN
static const uint32_t vcpu_ldo_dyn_temp_offsets[] = {VLD_OFFSETS(TEMP)};
static const uint32_t vcpu_ldo_dyn_current_offsets[] = {VLD_OFFSETS(CURRENT)};
static const uint32_t vcpu_ldo_dyn_ldo_offsets[] = {VLD_OFFSETS(LDO)};
static const uint32_t vcpu_ldo_dyn_vcpu_offsets[] = {VLD_OFFSETS(VCPU)};
static const uint32_t vcpu_ldo_dyn_temp_widths[] = {VLD_WIDTHS(TEMP)};
static const uint32_t vcpu_ldo_dyn_current_widths[] = {VLD_WIDTHS(CURRENT)};
static const uint32_t vcpu_ldo_dyn_ldo_widths[] = {VLD_WIDTHS(LDO)};
static const uint32_t vcpu_ldo_dyn_vcpu_widths[] = {VLD_WIDTHS(VCPU)};

// helpers for CDYN
#define CC(index, field, type) PFUSE(CORE_CDYN, index, field, type)
#define CC_ALL(f, t)           CC(0, f, t), CC(1, f, t), CC(2, f, t)
#define CC_OFFSETS(f)          CC_ALL(f, BIT_OFFSET)
#define CC_WIDTHS(f)           CC_ALL(f, WIDTH)

// arrays for CDYN
static const uint32_t core_cdyn_cdyn_offsets[] = {CC_OFFSETS(CDYN)};
static const uint32_t core_cdyn_ldo_offsets[] = {CC_OFFSETS(LDO)};
static const uint32_t core_cdyn_cdyn_widths[] = {CC_WIDTHS(CDYN)};
static const uint32_t core_cdyn_ldo_widths[] = {CC_WIDTHS(LDO)};

/*------------- Functions ----------------*/

bool power_fuses_is_power_hw_supported()
{
    return true;
}

int32_t platform_read_fuse(const uint32_t* target_addr, const uint32_t fuse_bit_offset, const uint32_t fuse_bit_size)
{
    uint64_t fuse_data = 0xdeadbeef;
    int status = 0;

    if ((fuse_bit_size == 0) || (fuse_bit_size > MAX_BITS_PER_FUSE))
    {
        POWER_LOG_INFO("Requested Fuse Size in bits not valid(Min:%d Max:%d bits)", 1, MAX_BITS_PER_FUSE);
        // FUSE_ET_READ(FUSE_STATUS_INVALID_SIZE, fuse_bit_offset, fuse_bit_size, 0);
        return FPFW_STATUS_INVALID_ARGS; // need E_PARAM define
    }

    fuse_data = fuse_read(fuse_bit_offset, fuse_bit_size);
    // number of valid bytes to copy from fuse_data
    size_t fuse_size = ((fuse_bit_size + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
    memcpy((void*)target_addr, (void*)&fuse_data, fuse_size);
    // FUSE_ET_READ(FUSE_STATUS_INVALID_SIZE, fuse_bit_offset, fuse_bit_size, 0);

    // Always return Success.
    status = FPFW_STATUS_SUCCESS;
    return status;
}

uint8_t power_fuses_get_pmm_rev()
{
    uint64_t fuse_data = 0;

    // read fuse for data
    int32_t status =
        platform_read_fuse((uint32_t*)&fuse_data, TEST_FLOW_CHECKS_PMM_REVISION_BIT_OFFSET, TEST_FLOW_CHECKS_PMM_REVISION_WIDTH);
    BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);

    const uint8_t pmm_rev = (uint8_t)fuse_data;
    // check supported revisions
    switch (pmm_rev)
    {
    case 0:
    case 1:
        // these are known PMM revisions supported by this file
        break;
    default:
        BUG_CHECK(KNG_SC_FUSE_UNSUPPORTED_PMMREV, pmm_rev, 0);
        break;
    }

    return pmm_rev;
}

int32_t power_fuses_get_dts_coeff(uint32_t k_offset,
                                  uint32_t k_width,
                                  uint32_t y_offset,
                                  uint32_t y_width,
                                  uint32_t fuse_elements,
                                  uint32_t coeff_count,
                                  uint32_t coeff_spacing,
                                  dts_coeff_t* dts_coeff)
{
    // verify input -- count of elements with the spacing we were given doesn't exceed count of fuse elements
    BUG_ASSERT_PARAM(((coeff_count * coeff_spacing) <= fuse_elements), (coeff_count * coeff_spacing), fuse_elements);
    BUG_ASSERT_PARAM((k_width > 0), k_width, 0);
    BUG_ASSERT_PARAM((y_width > 0), y_width, 0);
    BUG_ASSERT_PARAM((fuse_elements > 0), fuse_elements, 0);

    // check for NULL parameter
    if (!dts_coeff)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    // get pmm rev
    const uint8_t pmm_rev = power_fuses_get_pmm_rev();
    int32_t status;

    // temp space for fuse data
    uint64_t fuse_data_y = 0;
    uint64_t fuse_data_k = 0;

    for (uint32_t coeff_idx = 0; coeff_idx < coeff_count; ++coeff_idx)
    {
        // read fuses to get y/k for first idx (pmm_rev0) or all idx
        if ((pmm_rev == 1) || (coeff_idx == 0))
        {
            // read DTS Y value
            status = platform_read_fuse((uint32_t*)&fuse_data_y, y_offset, y_width);
            if (status != FPFW_STATUS_SUCCESS)
            {
                return status;
            }

            if (IS_PLATFORM_SVP() && fuse_data_y == 0)
            {
                fuse_data_y = 1;
            }
            // a value of 0 for Y is invalid; would cause a divide by 0
            if (fuse_data_y == 0)
            {
                POWER_LOG_CRIT(MODULE_NAME "DTS coeff y_val at 0x%x is 0", (unsigned int)y_offset);
                BUG_CHECK(KNG_SC_FUSE_DTS_Y, (uint16_t)fuse_data_y, y_offset);
            }

            // read DTS K value
            status = platform_read_fuse((uint32_t*)&fuse_data_k, k_offset, k_width);
            if (status != FPFW_STATUS_SUCCESS)
            {
                return status;
            }
        }

        dts_coeff[coeff_idx].y_val = (uint16_t)fuse_data_y;
        dts_coeff[coeff_idx].k_val = (uint16_t)fuse_data_k;

        // update fuse array offsets
        k_offset += (k_width * coeff_spacing);
        y_offset += (y_width * coeff_spacing);
    }
    return FPFW_STATUS_SUCCESS;
}

int32_t ldo_cap_to_max(uint32_t freq, uint32_t ldodac, uint32_t curve_idx)
{
    const uint32_t max_value = (1 << core_vft_fuse_ldodac_widths[curve_idx][0]) - 1;
    uint32_t ldodac_ret = ldodac;
    if (ldodac > max_value)
    {
        POWER_ET_WARN(POWER_ET_TYPE_FUSE_LDO_CAPPED, POWER_ET_ENCODE_FREQ_CURVE(freq, curve_idx));
        POWER_LOG_WARN(MODULE_NAME
                       "LDO + offset capped to max of %u (was %u fuse+offset) on curve %u at %uMHz point.",
                       (unsigned)max_value,
                       (uint16_t)ldodac,
                       (unsigned)curve_idx,
                       (unsigned)freq);
        ldodac_ret = max_value;
    }
    return ldodac_ret;
}

int32_t power_fuses_read_memasst(dvfs_core_memasst_entries_t* memasst_entries)
{
    // unexpected, but if something changes want to know about it
    // NOTE: The SCP FW is starting from 0x80 and 0x0 could contain garbage data hence commenting out below line
    BUG_ASSERT_PARAM((DIMOF((((dvfs_core_memasst_entries_t*)0)->entry)) >= DIMOF(core_memasst_tpemab_offsets)),
                     DIMOF((((dvfs_core_memasst_entries_t*)0)->entry)),
                     DIMOF(core_memasst_tpemab_offsets));

    if (!memasst_entries)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    // clear structure
    memset(memasst_entries, 0, sizeof(dvfs_core_memasst_entries_t));

    // iterate over fuses
    for (uint32_t entry_idx = 0; entry_idx < DIMOF(core_memasst_tpemab_offsets); ++entry_idx)
    {
        uint64_t fuse_data = 0;
        // read fuses at index
        int32_t status = platform_read_fuse((uint32_t*)&fuse_data,
                                            core_memasst_tpemab_offsets[entry_idx],
                                            core_memasst_tpemab_widths[entry_idx]);
        memasst_entries->entry[entry_idx].tp_emab = (uint8_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     core_memasst_tpemaa_offsets[entry_idx],
                                     core_memasst_tpemaa_widths[entry_idx]);
        memasst_entries->entry[entry_idx].tp_emaa = (uint8_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     core_memasst_hshcema_offsets[entry_idx],
                                     core_memasst_hshcema_widths[entry_idx]);
        memasst_entries->entry[entry_idx].hshc_ema = (uint8_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     core_memasst_hshcrawlm_offsets[entry_idx],
                                     core_memasst_hshcrawlm_widths[entry_idx]);
        memasst_entries->entry[entry_idx].hshc_rawlm = (uint8_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     core_memasst_hdema_offsets[entry_idx],
                                     core_memasst_hdema_widths[entry_idx]);
        memasst_entries->entry[entry_idx].hd_ema = (uint8_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     core_memasst_hdrawlm_offsets[entry_idx],
                                     core_memasst_hdrawlm_widths[entry_idx]);
        memasst_entries->entry[entry_idx].hd_rawlm = (uint8_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     core_memasst_hd_emaw_offsets[entry_idx],
                                     core_memasst_hd_emaw_widths[entry_idx]);
        memasst_entries->entry[entry_idx].hd_emaw = (uint8_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     core_memasst_ldodacin_offsets[entry_idx],
                                     core_memasst_ldodacin_widths[entry_idx]);
        memasst_entries->entry[entry_idx].ldo_dac_in = (uint16_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     core_memasst_valid_boundary_offsets[entry_idx],
                                     core_memasst_valid_boundary_widths[entry_idx]);
        memasst_entries->entry[entry_idx].valid_boundary = (uint8_t)fuse_data;

        // check status once, since we OR'd together
        if (status != FPFW_STATUS_SUCCESS)
        {
            return FPFW_STATUS_FAIL;
        }
    }
    return FPFW_STATUS_SUCCESS;
}

int32_t power_fuses_clear_core_valid_bits(corebits_t* valid_bits)
{
    uint32_t corebit = 0;
    uint32_t fuse_core_disable_st;
    if (!valid_bits)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    // iterate over fuses
    for (uint32_t fuse_idx = 0; fuse_idx < DIMOF(core_disables_fuse_offsets) / 2; ++fuse_idx)
    {
        uint64_t fuse_data = 0;

        // read fuse at index into temp fuse data
        int32_t status = platform_read_fuse((uint32_t*)&fuse_data,
                                            core_disables_fuse_offsets[2 * fuse_idx],
                                            core_disables_fuse_widths[2 * fuse_idx]);
        if (status != FPFW_STATUS_SUCCESS)
        {
            return status;
        }
        fuse_core_disable_st = fuse_data;
        status = platform_read_fuse((uint32_t*)&fuse_data,
                                    core_disables_fuse_offsets[2 * fuse_idx + 1],
                                    core_disables_fuse_widths[2 * fuse_idx + 1]);
        if (status != FPFW_STATUS_SUCCESS)
        {
            return status;
        }

        fuse_core_disable_st |= fuse_data;

        // iterate over each bit in fuse data (based on fuse width)
        for (uint32_t core_idx = 0; core_idx < core_disables_fuse_widths[2 * fuse_idx]; ++core_idx)
        {
            if ((fuse_core_disable_st >> core_idx) & 0x1)
            {
                // core is disabled, clear valid
                corebits_clear_bit(valid_bits, corebit);
            }
            // for every bit we check in disables, increase our global bit by
            // one
            ++corebit;
        }
    }
    return FPFW_STATUS_SUCCESS;
}

int32_t power_fuses_get_ldodac_to_voltage(dvfs_vf_slope_t* slope_offset)
{
    if (!slope_offset)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    uint64_t fuse_data = 0;
    int32_t status =
        platform_read_fuse((uint32_t*)&fuse_data, LDO_DAC_TO_VOLTAGE_M_BIT_OFFSET, LDO_DAC_TO_VOLTAGE_M_WIDTH);

    // a value of 0 for gradient is invalid
    // workaround
    // TODO: Need to remove the default value assignemnt M value calculation needs to be investigated
    if (IS_PLATFORM_SVP() && fuse_data == 0)
    {
        fuse_data = 1;
    }

    if ((fuse_data == 0) || (status != FPFW_STATUS_SUCCESS))
    {
        POWER_LOG_CRIT(MODULE_NAME "ldodac_to_voltage gradient is 0");
        BUG_CHECK(KNG_SC_FUSE_L2V_SLOPE, (uint16_t)fuse_data, 0);
    }
    slope_offset->slope_uvolt = (uint16_t)fuse_data;

    status = platform_read_fuse((uint32_t*)&fuse_data,
                                LDO_DAC_TO_VOLTAGE_B_BIT_OFFSET,
                                LDO_DAC_TO_VOLTAGE_B_WIDTH); // 0mV offset (-2000uV to allow for negatives)
    if (status != FPFW_STATUS_SUCCESS)
    {
        return status;
    }
    slope_offset->offset_uvolt = (uint16_t)fuse_data;

    return FPFW_STATUS_SUCCESS;
}

int32_t power_fuses_get_dts_coeff_tile(dts_coeff_t* dts_coeff, uint32_t count)
{
    return power_fuses_get_dts_coeff(TILE_THERMALS_SENSOR_RTS_K_BIT_OFFSET,
                                     TILE_THERMALS_SENSOR_RTS_K_WIDTH,
                                     TILE_THERMALS_SENSOR_RTS_Y_BIT_OFFSET,
                                     TILE_THERMALS_SENSOR_RTS_Y_WIDTH,
                                     TILE_THERMALS_SENSOR_RTS_Y_ARRAY_ELEMENTS,
                                     count,
                                     TILE_PVT_NUM_CHANNELS_DTS,
                                     dts_coeff);
}

int32_t power_fuses_get_dts_coeff_soctop(dts_coeff_t* dts_coeff, uint32_t count)
{
    return power_fuses_get_dts_coeff(TOP_THERMALS_SENSOR_RTS_K_BIT_OFFSET,
                                     TOP_THERMALS_SENSOR_RTS_K_WIDTH,
                                     TOP_THERMALS_SENSOR_RTS_Y_BIT_OFFSET,
                                     TOP_THERMALS_SENSOR_RTS_Y_WIDTH,
                                     TOP_THERMALS_SENSOR_RTS_Y_ARRAY_ELEMENTS,
                                     count,
                                     1,
                                     dts_coeff);
}

int32_t power_fuses_get_ldo_headroom(uint8_t* ldo_headroom)
{
    if (!ldo_headroom)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    uint64_t fuse_data = 0;
    // read fuse for data
    int32_t status = platform_read_fuse((uint32_t*)&fuse_data, LDO_HEADROOM_BIT_OFFSET, LDO_HEADROOM_WIDTH);
    if (status != FPFW_STATUS_SUCCESS)
    {
        return status;
    }
    *ldo_headroom = (uint8_t)fuse_data;

    return FPFW_STATUS_SUCCESS;
}

int32_t power_fuses_get_vcpu_guardband(uint8_t* vcpu_guardband)
{
    if (!vcpu_guardband)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    uint64_t fuse_data = 0;
    // read fuse for data
    int32_t status = platform_read_fuse((uint32_t*)&fuse_data, VCPU_GUARDBAND_BIT_OFFSET, VCPU_GUARDBAND_WIDTH);
    if (status != FPFW_STATUS_SUCCESS)
    {
        return status;
    }
    *vcpu_guardband = (uint8_t)fuse_data;

    return FPFW_STATUS_SUCCESS;
}

int32_t power_fuses_vcpu_leakage(power_vcpu_interp_t* vcpu_leakage, uint32_t count)
{
    // unexpected, but if something changes want to know about it
    BUG_ASSERT_PARAM((count >= DIMOF(vcpu_leakage_temp_offsets)), count, DIMOF(vcpu_leakage_temp_offsets));

    if (!vcpu_leakage)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    // clear structure
    memset(vcpu_leakage, 0, sizeof(power_vcpu_interp_t) * count);

    // iterate over fuses
    for (uint32_t entry_idx = 0; entry_idx < DIMOF(vcpu_leakage_temp_offsets); ++entry_idx)
    {
        uint64_t fuse_data = 0;
        // read fuses at index
        int32_t status = platform_read_fuse((uint32_t*)&fuse_data,
                                            vcpu_leakage_temp_offsets[entry_idx],
                                            vcpu_leakage_temp_widths[entry_idx]);
        vcpu_leakage[entry_idx].current.temp_offset = (uint8_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     vcpu_leakage_current_offsets[entry_idx],
                                     vcpu_leakage_current_widths[entry_idx]);
        vcpu_leakage[entry_idx].current.current_ma = (uint32_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     vcpu_leakage_vcpu_offsets[entry_idx],
                                     vcpu_leakage_vcpu_widths[entry_idx]);
        vcpu_leakage[entry_idx].current.vcpu_mv = (uint16_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     vcpu_leakage_ldo_offsets[entry_idx],
                                     vcpu_leakage_ldo_widths[entry_idx]);
        vcpu_leakage[entry_idx].current.ldo_dac = (uint16_t)fuse_data;

        // check status once, since we OR'd together
        if (status != FPFW_STATUS_SUCCESS)
        {
            return FPFW_STATUS_FAIL;
        }
    }
    return FPFW_STATUS_SUCCESS;
}

int32_t power_fuses_ldo_dyn(power_vcpu_interp_t* vcpu_ldo_dyn, uint32_t count)
{
    // unexpected, but if something changes want to know about it
    BUG_ASSERT_PARAM((count >= DIMOF(vcpu_ldo_dyn_temp_offsets)), count, DIMOF(vcpu_ldo_dyn_temp_offsets));

    if (!vcpu_ldo_dyn)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    // clear structure
    memset(vcpu_ldo_dyn, 0, sizeof(power_vcpu_interp_t) * count);

    // iterate over fuses
    for (uint32_t entry_idx = 0; entry_idx < DIMOF(vcpu_ldo_dyn_temp_offsets); ++entry_idx)
    {
        uint64_t fuse_data = 0;
        // read fuses at index
        int32_t status = platform_read_fuse((uint32_t*)&fuse_data,
                                            vcpu_ldo_dyn_temp_offsets[entry_idx],
                                            vcpu_ldo_dyn_temp_widths[entry_idx]);
        vcpu_ldo_dyn[entry_idx].current.temp_offset = (uint8_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     vcpu_ldo_dyn_current_offsets[entry_idx],
                                     vcpu_ldo_dyn_current_widths[entry_idx]);
        vcpu_ldo_dyn[entry_idx].current.current_ma = (uint32_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     vcpu_ldo_dyn_vcpu_offsets[entry_idx],
                                     vcpu_ldo_dyn_vcpu_widths[entry_idx]);
        vcpu_ldo_dyn[entry_idx].current.vcpu_mv = (uint16_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data,
                                     vcpu_ldo_dyn_ldo_offsets[entry_idx],
                                     vcpu_ldo_dyn_ldo_widths[entry_idx]);
        vcpu_ldo_dyn[entry_idx].current.ldo_dac = (uint16_t)fuse_data;

        // check status once, since we OR'd together
        if (status != FPFW_STATUS_SUCCESS)
        {
            return FPFW_STATUS_FAIL;
        }
    }
    return FPFW_STATUS_SUCCESS;
}

int32_t power_fuses_core_cdyn(power_vcpu_interp_t* core_cdyn, uint32_t count)
{
    // unexpected, but if something changes want to know about it
    BUG_ASSERT_PARAM((count >= DIMOF(core_cdyn_cdyn_offsets)), count, DIMOF(core_cdyn_cdyn_offsets));

    if (!core_cdyn)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    // clear structure
    memset(core_cdyn, 0, sizeof(power_vcpu_interp_t) * count);

    // iterate over fuses
    for (uint32_t entry_idx = 0; entry_idx < DIMOF(core_cdyn_cdyn_offsets); ++entry_idx)
    {
        uint64_t fuse_data = 0;
        // read fuses at index
        int32_t status =
            platform_read_fuse((uint32_t*)&fuse_data, core_cdyn_cdyn_offsets[entry_idx], core_cdyn_cdyn_widths[entry_idx]);
        core_cdyn[entry_idx].cdyn.cdyn_pf = (uint16_t)fuse_data;
        status |= platform_read_fuse((uint32_t*)&fuse_data, core_cdyn_ldo_offsets[entry_idx], core_cdyn_ldo_widths[entry_idx]);
        core_cdyn[entry_idx].cdyn.ldo_dac = (uint16_t)fuse_data;

        // check status once, since we OR'd together
        if (status != FPFW_STATUS_SUCCESS)
        {
            return FPFW_STATUS_FAIL;
        }
    }
    return FPFW_STATUS_SUCCESS;
}

int32_t power_fuses_process_id(power_fuse_process_id_t* process_id)
{
    if (!process_id)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    uint64_t fuse_data = 0;
    // read fuse for data
    int32_t status = platform_read_fuse((uint32_t*)&fuse_data,
                                        PROCESS_CORNER_IDENTIFIER_INVERTER_PROCESS_ID_BIT_OFFSET,
                                        PROCESS_CORNER_IDENTIFIER_INVERTER_PROCESS_ID_WIDTH);
    if (status != FPFW_STATUS_SUCCESS)
    {
        return status;
    }
    *process_id = (power_fuse_process_id_t)fuse_data;

    return FPFW_STATUS_SUCCESS;
}

int32_t power_fuses_get_tdp_config(power_fuse_tdp_t* tdp_config)
{
    if (!tdp_config)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    uint64_t fuse_data = 0;

    // read fuse for data
    int32_t status = platform_read_fuse((uint32_t*)&fuse_data, NUM_CORES_BIT_OFFSET, NUM_CORES_WIDTH);

    if (IS_PLATFORM_SVP() && fuse_data == 0)
    {
        fuse_data = 124;
    }

    if ((fuse_data == 0) || (status != FPFW_STATUS_SUCCESS))
    {
        // we divide by num cores, so it can't be 0
        POWER_LOG_CRIT(MODULE_NAME "TDP num cores is 0");
        BUG_CHECK(KNG_SC_FUSE_TDP_NUM_CORES, (uint16_t)fuse_data, 0);
    }
    tdp_config->num_cores = (uint8_t)fuse_data;

    status = platform_read_fuse((uint32_t*)&fuse_data, BASE_FREQUENCY_BIT_OFFSET, BASE_FREQUENCY_WIDTH);
    if (status != FPFW_STATUS_SUCCESS)
    {
        return status;
    }
    tdp_config->freq_MHz = (uint16_t)fuse_data;
    tdp_config->power_A = TDP_DEFAULT_POWER_A;

    return FPFW_STATUS_SUCCESS;
}

int32_t power_fuses_get_curve_assignment(uint32_t core, uint32_t* curve_assignment)
{
    // ensure unexpected doesn't occur
    BUG_ASSERT_PARAM((core < VF_CORE_ANCHOR_SEL_ARRAY_ELEMENTS), core, VF_CORE_ANCHOR_SEL_ARRAY_ELEMENTS);
    if (!curve_assignment)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    uint64_t fuse_data = 0;
    // read fuse for data
    int32_t status = platform_read_fuse((uint32_t*)&fuse_data,
                                        VF_CORE_ANCHOR_SEL_BIT_OFFSET + (core * VF_CORE_ANCHOR_SEL_WIDTH),
                                        VF_CORE_ANCHOR_SEL_WIDTH);
    if (status != FPFW_STATUS_SUCCESS)
    {
        return status;
    }
    *curve_assignment = (uint32_t)fuse_data;

    return FPFW_STATUS_SUCCESS;
}

// NOTE: Currently the temperature curves are not considered and temp0 is used statically and hence hardcoded
int32_t power_fuses_read_vf(power_fuse_vf_curveset_t* vf_curves, int8_t ldo_offset)
{
    // unexpected, but if something changes want to know about it
    // Checking that the size of pair index in the innermost struct matches with our array definitions
    BUG_ASSERT_PARAM(
        (DIMOF(
             ((power_fuse_vf_curveset_t*)0)->curveset[VFT_CURVESET_COUNT].curve[VFT_CURVE_COUNT_PER_CURVESET].pair) >=
         DIMOF(core_vft_fuse_freq_offsets)),
        DIMOF(((power_fuse_vf_curveset_t*)0)->curveset[VFT_CURVESET_COUNT].curve[VFT_CURVE_COUNT_PER_CURVESET].pair),
        DIMOF(core_vft_fuse_freq_offsets));
    BUG_ASSERT_PARAM((DVFS_FUSED_PAIRS_COUNT >= VFT_CURVESET_COUNT), DVFS_FUSED_PAIRS_COUNT, VFT_CURVESET_COUNT);

    if (!vf_curves)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    if (ldo_offset != 0)
    {
        POWER_ET_WARN(POWER_ET_TYPE_FUSE_LDO_OFFSET, ldo_offset);
        POWER_LOG_WARN(MODULE_NAME "Fused LDO values offset by %d\n", ldo_offset);
    }

    // clear structure
    memset(vf_curves, 0, sizeof(power_fuse_vf_curveset_t));
    // iterate over fuses
    for (uint32_t curve_idx = 0; curve_idx < DIMOF(core_vft_fuse_freq_offsets); ++curve_idx)
    {
        for (uint32_t temp_idx = 0; temp_idx < VFT_CURVE_COUNT_PER_CURVESET; ++temp_idx)
        {
            uint64_t fuse_data = 0;
            for (uint32_t pair_idx = 0; pair_idx < VFT_CURVESET_COUNT; ++pair_idx)
            {

                // read fuse at index into temp fuse data
                int32_t status = platform_read_fuse(
                    (uint32_t*)&fuse_data,
                    core_vft_fuse_freq_offsets[curve_idx][temp_idx + pair_idx * VFT_CURVE_COUNT_PER_CURVESET],
                    core_vft_fuse_freq_widths[curve_idx][temp_idx + pair_idx * VFT_CURVE_COUNT_PER_CURVESET]);
                if (status != FPFW_STATUS_SUCCESS)
                {
                    return status;
                }
                vf_curves->curveset[curve_idx].curve[temp_idx].pair[pair_idx].freq_Mhz = fuse_data;

                // read ldodaccode fuse
                status = platform_read_fuse(
                    (uint32_t*)&fuse_data,
                    core_vft_fuse_ldodac_offsets[curve_idx][temp_idx + pair_idx * VFT_CURVE_COUNT_PER_CURVESET],
                    core_vft_fuse_ldodac_widths[curve_idx][temp_idx + pair_idx * VFT_CURVE_COUNT_PER_CURVESET]);
                if (status != FPFW_STATUS_SUCCESS)
                {
                    return status;
                }
                if (fuse_data != 0)
                {
                    // offset ldo value if offset provided, cap to max ldo value
                    fuse_data = ldo_cap_to_max(vf_curves->curveset[curve_idx].curve[temp_idx].pair[pair_idx].freq_Mhz,
                                               fuse_data + ldo_offset,
                                               curve_idx);
                }
                vf_curves->curveset[curve_idx].curve[temp_idx].pair[pair_idx].ldo_dac_in = fuse_data;
            }
        }
    }
    return FPFW_STATUS_SUCCESS;
}

int32_t power_fuses_get_curve_temp(int8_t* core_max_temp, uint32_t count)
{
    if (!core_max_temp)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    if (count != (NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS - 1))
    {
        return FPFW_STATUS_INVALID_ARGS;
    }

    memset(core_max_temp, 0, count);
    // clear structure
    FPFW_UNUSED(count);
    int32_t status = 0;
    // iterate over fuses

    for (uint32_t entry_idx = 0; entry_idx < DIMOF(curve_temp_ranges_offsets); entry_idx++)
    {
        uint64_t fuse_data = 0;
        // read fuses at index
        status = platform_read_fuse((uint32_t*)&fuse_data,
                                    curve_temp_ranges_offsets[entry_idx],
                                    curve_temp_ranges_widths[entry_idx]);

        if (status != FPFW_STATUS_SUCCESS)
        {
            return status;
        }

        core_max_temp[entry_idx] = (int8_t)fuse_data;
        core_max_temp[entry_idx] -= VF_CURVE_TEMP_RANGE_OFFSET;
    }

    return FPFW_STATUS_SUCCESS;
}

int32_t power_fuses_get_vsys_vid(uint16_t* vsys_vid)
{
    if (!vsys_vid)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    uint64_t fuse_data = 0;

    // read fuse for data
    int32_t status = platform_read_fuse((uint32_t*)&fuse_data, VSYS_VID_BIT_OFFSET, VSYS_VID_WIDTH);
    if (status != FPFW_STATUS_SUCCESS)
    {
        *vsys_vid = 0;
        return status;
    }

    *vsys_vid = (uint16_t)fuse_data;
    return FPFW_STATUS_SUCCESS;
}

void power_fuses_read(power_fuse_data_t* p_fuses)
{
    power_runconfig_t* p_run_config = power_runconfig_get();

    /* temp implementation for fuse reads */
    BUG_ASSERT_PARAM((p_fuses != NULL), (uint32_t)p_fuses, (uint32_t)NULL);

    const uint32_t core_count = NUM_AP_CORES_PER_DIE; // system_info_get_core_count();

    // set all the valid bits for platform cores; fuses may clear further in
    for (uint32_t core_idx = 0; core_idx < core_count; ++core_idx)
    {
        corebits_set_bit(&p_fuses->valid_cores, core_idx);
    }

    // read fuses on platforms that support reads
    if (power_fuses_is_power_hw_supported())
    {
        int32_t status = FPFW_STATUS_SUCCESS;
        // solve the bug2762916 [C4143] Core Disable Configuration Register doesn't reflect the core_defect_mask bit in fuses
        memcpy(&p_fuses->valid_cores, core_info_get_enable_cores_result(), sizeof(corebits_t));
        status = power_fuses_read_vf(&p_fuses->vf, p_run_config->knobs.ldo_offset);
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_read_memasst(&p_fuses->memasst);
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_get_ldodac_to_voltage(&p_fuses->ldodac_to_volt);
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_get_dts_coeff_tile(&p_fuses->dts_coeff_tile[0], ARRAY_SIZE(p_fuses->dts_coeff_tile));
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_get_dts_coeff_soctop(&p_fuses->dts_coeff_soctop[0], ARRAY_SIZE(p_fuses->dts_coeff_soctop));
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_get_ldo_headroom(&p_fuses->v_ldo_dropout_mv); // read fuse - 1
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_get_vcpu_guardband(&p_fuses->vcpu_guardband_mv); // read fuse - 1
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_vcpu_leakage(&p_fuses->vcpu_leakage[0], DIMOF(p_fuses->vcpu_leakage));
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_ldo_dyn(&p_fuses->vcpu_ldo_dyn[0], DIMOF(p_fuses->vcpu_ldo_dyn));
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_core_cdyn(&p_fuses->core_cdyn[0], DIMOF(p_fuses->core_cdyn));
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_process_id(&p_fuses->process_id); // read fuse - 1
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_get_tdp_config(&p_fuses->tdp_config); // read fuse - 3
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_get_curve_temp(&p_fuses->curve_max_temp[0], ARRAY_SIZE(p_fuses->curve_max_temp));
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
        status = power_fuses_get_vsys_vid(&p_fuses->vsys_vid_mv);
        BUG_ASSERT_PARAM((status == FPFW_STATUS_SUCCESS), status, FPFW_STATUS_SUCCESS);
    }
    else
    {
        // default gradient/offset parameters
        const dvfs_vf_slope_t default_ldodac_to_volt = {
            .slope_uvolt = DVFS_LDODAC_TO_VOLT_SLOPE,
            .offset_uvolt = DVFS_LDODAC_TO_VOLT_OFFSET,
        };
        p_fuses->ldodac_to_volt = default_ldodac_to_volt;
        // default dts_coeff parameters
        const dts_coeff_t default_dts_coeff = {
            .k_val = PVT_DTS_DEFAULT_K_VAL,
            .y_val = PVT_DTS_DEFAULT_Y_VAL,
        };
        for (uint32_t idx = 0; idx < ARRAY_SIZE(p_fuses->dts_coeff_tile); ++idx)
        {
            p_fuses->dts_coeff_tile[idx] = default_dts_coeff;
        }
        for (uint32_t idx = 0; idx < ARRAY_SIZE(p_fuses->dts_coeff_soctop); ++idx)
        {
            p_fuses->dts_coeff_soctop[idx] = default_dts_coeff;
        }

        // need to default num cores
        const power_fuse_tdp_t default_tdp_config = {.num_cores = NUM_AP_CORES_PER_DIE,
                                                     .freq_MHz = DVFS_DEF_NOMINAL_FREQ,
                                                     .power_A = TDP_DEFAULT_POWER_A};
        p_fuses->tdp_config = default_tdp_config;
    }
}
