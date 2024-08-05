//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_test.cpp
 * Unit test file for accelerator_ip_test lib
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep

extern "C" {
#include "accelip_ss_init.h"
#include "atu_lib.h"           // for ATU_ID_MAX, atu_id_t, atu_map_entry_t
#include "kng_soc_constants.h" // for SOC_D0, SDMSS_INSTANCE
#include "silibs_status.h"     // for SILIBS_SUCCESS, SILIBS_E_PARAM

#include <accelerator_ip.h> // for scp_accelerators_init, ACCEL_RET_SUCCESS
#include <idsw_kng.h>       // for KNG_DIE_ID
#include <pcr_rpss.h>       // for pcr_rpss_entity_t
#include <stdint.h>         // for uintptr_t, uint32_t, uint8_t, uint64_t
#include <utils.h>          // for UNUSED

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == nullptr)
    {
        return SILIBS_E_PARAM;
    }

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == nullptr)
    {
        return SILIBS_E_PARAM;
    }

    return mock_type(int);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv()
{
    return mock_type(KNG_PLAT_ID);
}

silibs_status_t __wrap_accelip_ss_init(uintptr_t base_addr, ACCELIP_SS_INSTANCE accelip_id, accelip_ss_init_t* init)
{
    UNUSED(base_addr);
    UNUSED(accelip_id);
    UNUSED(init);

    return mock_type(silibs_status_t);
}

int __wrap_pcr_rpss_init_entity(pcr_rpss_entity_t* pcr, uint16_t enabled_clocks, uintptr_t base)
{
    UNUSED(pcr);
    UNUSED(enabled_clocks);
    UNUSED(base);

    return SILIBS_SUCCESS;
}

int __wrap_pcr_rpss_configure_clock(pcr_rpss_entity_t* pcr, uint32_t us_timeout)
{
    UNUSED(pcr);
    UNUSED(us_timeout);

    return SILIBS_SUCCESS;
}

int __wrap_pcr_rpss_deassert_por_reset(pcr_rpss_entity_t* pcr)
{
    UNUSED(pcr);

    return mock_type(int);
}

void __wrap_debug_print(const char* fmt, ...)
{
    UNUSED(fmt);
}

void __wrap_critical_print(const char* fmt, ...)
{
    UNUSED(fmt);
}

void __wrap_configure_cdedss_hsp_system_addr_map(const uint64_t hsp_base_addr, uintptr_t tower_base_addr)
{
    UNUSED(hsp_base_addr);
    UNUSED(tower_base_addr);
}

void __wrap_FpFwAssert(int expression)
{
    UNUSED(expression);
}

void __wrap_configure_cdedss_vab_system_addr_map(CDEDSS_INSTANCE cdedss_id, uintptr_t tower_base_addr)
{
    UNUSED(cdedss_id);
    UNUSED(tower_base_addr);
}

TEST_FUNCTION(accelip_pre_boot_config_pass_test, nullptr, nullptr)
{
    // Happy case setup
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    // init accelertor
    will_return_count(__wrap_atu_map, SILIBS_SUCCESS, 3);
    will_return_count(__wrap_accelip_ss_init, SILIBS_SUCCESS, 2);
    will_return_count(__wrap_atu_unmap, SILIBS_SUCCESS, 3);

    assert_int_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_pre_boot_config_atu_map_fail_test, nullptr, nullptr)
{
    // ATU MAP fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    will_return_count(__wrap_atu_map, SILIBS_E_PARAM, 2);

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_pre_boot_config_atu_unmap_fail_test, nullptr, nullptr)
{
    // ATU UNMAP fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    will_return_count(__wrap_atu_map, SILIBS_SUCCESS, 2);
    will_return(__wrap_accelip_ss_init, SILIBS_SUCCESS);
    will_return_count(__wrap_atu_unmap, SILIBS_E_PARAM, 2);

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_pre_boot_config_accelip_ss_init_fail_test, nullptr, nullptr)
{
    // Accelip ss init fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    will_return_count(__wrap_atu_map, SILIBS_SUCCESS, 3);
    will_return_count(__wrap_accelip_ss_init, SILIBS_E_PARAM, 2);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}
}
