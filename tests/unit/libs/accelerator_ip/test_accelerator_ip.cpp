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
#include "atu_lib.h"           // for ATU_ID_MAX, atu_id_t, atu_map_entry_t
#include "kng_soc_constants.h" // for SOC_D0, SDMSS_INSTANCE
#include "sdm_init.h"          // for sdm_mem_init_t
#include "sdm_init_knobs.h"    // for sdm_pre_pcie_cfg_t
#include "silibs_kng_soc.h"    // for D0_SDM_RCIEP_BUS, D1_CDED_RCEC_BUS
#include "silibs_status.h"     // for SILIBS_SUCCESS, SILIBS_E_PARAM
#include "smmu_knobs.h"        // for smmu_gbpa_cfg_t

#include <accelerator_ip.h> // for scp_accelerators_init, ACCEL_RET_SUCCESS
#include <idsw.h>           // for DIE_ID, _PLAT_ID
#include <idsw_kng.h>       // for KNG_DIE_ID
#include <pcr_rpss.h>       // for pcr_rpss_entity_t
#include <smmu.h>           // for smmu_gpba_cfg
#include <stddef.h>         // for NULL
#include <stdint.h>         // for uintptr_t, uint32_t, uint8_t, uint64_t
#include <utils.h>          // for UNUSED

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
void __wrap_mmio_write8(volatile uint8_t* addr, uint8_t data)
{
    UNUSED(addr);
    UNUSED(data);
}

void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    UNUSED(addr);
    UNUSED(data);
}

void __wrap_mmio_update32(volatile uint32_t* addr, uint32_t data, uint32_t mask)
{
    UNUSED(addr);
    UNUSED(data);
    UNUSED(mask);
}

void __wrap_debug_print(const char* fmt, ...)
{
    UNUSED(fmt);
}

void __wrap_critical_print(const char* fmt, ...)
{
    UNUSED(fmt);
}

int __wrap_sdm_init_smmu_connection_seq(const uintptr_t ext_cfg_addr)
{
    if (ext_cfg_addr == 0x0)
    {
        return SILIBS_E_PARAM;
    }

    return SILIBS_SUCCESS;
}

int __wrap_sdm_init_set_ecam_base(const uintptr_t ext_cfg_addr, const uint64_t ecam_base_addr)
{
    if ((ext_cfg_addr == 0x0) || (ecam_base_addr != PCIE_ECAM_START))
    {
        return SILIBS_E_PARAM;
    }
    return SILIBS_SUCCESS;
}

int __wrap_sdm_init_set_rciep_bdf(const uintptr_t ext_cfg_addr, const uint8_t bus, const uint8_t dev, const uint8_t func)
{
    if ((ext_cfg_addr == 0x0) || (bus < D0_SDM_RCIEP_BUS || bus > D1_CDED_RCEC_BUS) || (dev != 0))
    {
        return SILIBS_E_PARAM;
    }

    // Valid range of func in case of PF and VF are not clear so not checking
    UNUSED(func);

    return SILIBS_SUCCESS;
}

int __wrap_sdm_init_enable_ecam(const uintptr_t ext_cfg_addr, const bool enable)
{
    UNUSED(enable);

    if (ext_cfg_addr == 0x0)
    {
        return SILIBS_E_PARAM;
    }

    return SILIBS_SUCCESS;
}

int __wrap_sdm_init_write_pre_pcie_cfg(const uintptr_t ext_cfg_addr, const sdm_pre_pcie_cfg_t* pre_pcie_cfg)
{
    if ((ext_cfg_addr == 0x0) || (pre_pcie_cfg == NULL))
    {
        return SILIBS_E_PARAM;
    }

    return SILIBS_SUCCESS;
}

int __wrap_sdm_init_set_emcpu_initvtor(const uintptr_t ext_cfg_addr, const uintptr_t addr)
{
    if ((ext_cfg_addr == 0x0) || (addr == 0x0))
    {
        return SILIBS_E_PARAM;
    }
    return SILIBS_SUCCESS;
}

int __wrap_sdm_init_enable_fence(const uintptr_t ext_cfg_addr, const bool enable)
{
    UNUSED(enable);

    if (ext_cfg_addr == 0x0)
    {
        return SILIBS_E_PARAM;
    }

    return SILIBS_SUCCESS;
}

int __wrap_sdm_init_deassert_nsysreset(const uintptr_t ext_cfg_addr)
{
    if (ext_cfg_addr == 0x0)
    {
        return SILIBS_E_PARAM;
    }

    return SILIBS_SUCCESS;
}

int __wrap_sdm_init_trigger_memory_init_blocking(const uintptr_t ext_cfg_addr, const sdm_mem_init_t* selector)
{
    if ((ext_cfg_addr == 0x0) || (selector == nullptr))
    {
        return SILIBS_E_PARAM;
    }

    return SILIBS_SUCCESS;
}

int __wrap_sdm_init_enable_itcm_ecc(const uintptr_t ext_cfg_addr, const bool enable)
{
    UNUSED(enable);

    if (ext_cfg_addr == 0x0)
    {
        return SILIBS_E_PARAM;
    }

    return SILIBS_SUCCESS;
}

int __wrap_sdm_init_enable_dtcm_ecc(const uintptr_t ext_cfg_addr, const bool enable)
{
    UNUSED(enable);

    if (ext_cfg_addr == 0x0)
    {
        return SILIBS_E_PARAM;
    }

    return SILIBS_SUCCESS;
}

int __wrap_sdm_init_disable_cpu_wait(const uintptr_t ext_cfg_addr)
{
    if (ext_cfg_addr == 0x0)
    {
        return SILIBS_E_PARAM;
    }

    return SILIBS_SUCCESS;
}

int __wrap_sdm_init_set_rcec_bdf(const uintptr_t ext_cfg_addr, const uint8_t bus, const uint8_t dev, const uint8_t func)
{
    UNUSED(ext_cfg_addr);

    if ((ext_cfg_addr == 0x0) || (bus < D0_SDM_RCIEP_BUS || bus > D1_CDED_RCEC_BUS) || (dev != 0))
    {
        return SILIBS_E_PARAM;
    }

    // Valid range of func in case of PF and VF are not clear so not checking
    UNUSED(func);

    return SILIBS_SUCCESS;
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_configure_vab_system_addr_map(uint64_t vab_base_addr, uint64_t tower_base_addr)
{
    UNUSED(vab_base_addr);
    UNUSED(tower_base_addr);
}

void __wrap_smmu_enable_access(uintptr_t smmu_base_addr)
{
    UNUSED(smmu_base_addr);
}

int __wrap_smmu_configure_gbpa(uintptr_t smmu_tcu_base, smmu_gbpa_cfg_t* smmu_gbpa_cfg, SECURITY_STATE security_state)
{
    UNUSED(smmu_tcu_base);
    UNUSED(smmu_gbpa_cfg);
    UNUSED(security_state);

    return mock_type(int);
}

bool __wrap_smmu_configure_gbpa_check(uintptr_t smmu_tcu_base, SECURITY_STATE security_state)
{
    UNUSED(smmu_tcu_base);
    UNUSED(security_state);

    return mock_type(bool);
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

bool __wrap_smmu_enable_access_check(uintptr_t smmu_base_addr)
{
    smmu_base_addr = mock_type(uintptr_t);

    if (smmu_base_addr == 0x0)
    {
        return false;
    }

    return true;
}

idsw_plat_id_t __wrap_idsw_get_platform_sdv()
{
    return mock_type(idsw_plat_id_t);
}

void __wrap_deassert_pcr_reset(uintptr_t vab_pcr_base_addr)
{
    UNUSED(vab_pcr_base_addr);
}

int __wrap_vab_pcr_init(uintptr_t vab_pcr_base_addr)
{
    vab_pcr_base_addr = mock_type(uintptr_t);

    if (vab_pcr_base_addr == 0x0)
    {
        return SILIBS_E_PARAM;
    }

    return SILIBS_SUCCESS;
}

void __wrap_FpFwAssert(int expression)
{
    UNUSED(expression);
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

void __wrap_configure_cdedss_hsp_system_addr_map(const uint64_t hsp_base_addr, uintptr_t tower_base_addr)
{
    UNUSED(hsp_base_addr);
    UNUSED(tower_base_addr);
}

void __wrap_configure_cdedss_vab_system_addr_map(CDEDSS_INSTANCE cdedss_id, uintptr_t tower_base_addr)
{
    UNUSED(cdedss_id);
    UNUSED(tower_base_addr);
}

void config_ss_pass()
{
    // for D0 SDMSS
    will_return(__wrap_atu_map, SILIBS_SUCCESS);                     // SS Tower atu_map
    will_return(__wrap_pcr_rpss_deassert_por_reset, SILIBS_SUCCESS); // PCR RPSS
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);                   // SS Tower atu_unmap
}

void config_ss_cdedss_pass()
{
    // for D0 CDEDSS
    will_return(__wrap_atu_map, SILIBS_SUCCESS);                     // SS Tower atu_map
    will_return(__wrap_pcr_rpss_deassert_por_reset, SILIBS_SUCCESS); // PCR RPSS
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);                   // SS Tower atu_unmap
}

void config_accel_ip_pass()
{
    // for D0 SDMSS
    will_return(__wrap_atu_map, SILIBS_SUCCESS);   // configure_accel_ip atu_map
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // configure_accel_ip atu_unmap
}

void config_accel_ip_cdedss_pass()
{
    // for D0 CDEDSS
    will_return(__wrap_atu_map, SILIBS_SUCCESS);   // configure_accel_ip atu_map
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // configure_accel_ip atu_unmap
}

TEST_FUNCTION(accel_ip_pre_boot_config_pass_test, nullptr, nullptr)
{
    // Setup
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    // Happy case for SDMSS
    config_ss_pass();
    config_accel_ip_pass();

    // Happy case for CDEDSS
    config_ss_cdedss_pass();
    config_accel_ip_cdedss_pass();

    assert_int_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accel_ip_pre_boot_config_ss_atu_map_fail_test, nullptr, nullptr)
{
    // Setup
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    // SS Tower atu_map fail for SDMSS
    will_return(__wrap_atu_map, SILIBS_E_PARAM); // SS Tower atu_map

    // SS Tower atu_map fail for CDEDSS
    will_return(__wrap_atu_map, SILIBS_E_PARAM); // SS Tower atu_map

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accel_ip_pre_boot_config_ss_pcr_init_fail_test, nullptr, nullptr)
{
    // Setup
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    // SS Tower atu_map success, SS pcr fail for SDMSS
    will_return(__wrap_atu_map, SILIBS_SUCCESS);                     // SS Tower atu_map
    will_return(__wrap_pcr_rpss_deassert_por_reset, SILIBS_E_PARAM); // PCR RPSS FAIL
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);                   // SS Tower atu_unmap

    // SS Tower atu_map success, SS pcr fail for CDEDSS
    will_return(__wrap_atu_map, SILIBS_SUCCESS);                     // SS Tower atu_map
    will_return(__wrap_pcr_rpss_deassert_por_reset, SILIBS_E_PARAM); // PCR RPSS FAIL
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);                   // SS Tower atu_unmap

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accel_ip_pre_boot_config_ss_atu_unmap_fail_test, nullptr, nullptr)
{
    // Setup
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    // SS Tower atu_map success, SS Tower atu_unmap fail for SDMSS
    will_return(__wrap_atu_map, SILIBS_SUCCESS);                     // SS Tower atu_map
    will_return(__wrap_pcr_rpss_deassert_por_reset, SILIBS_SUCCESS); // PCR RPSS

    will_return(__wrap_atu_unmap, SILIBS_E_PARAM); // SS Tower atu_unmap

    // SS Tower atu_map success, SS Tower atu_unmap fail for CDEDSS
    will_return(__wrap_atu_map, SILIBS_SUCCESS);                     // SS Tower atu_map
    will_return(__wrap_pcr_rpss_deassert_por_reset, SILIBS_SUCCESS); // PCR RPSS

    will_return(__wrap_atu_unmap, SILIBS_E_PARAM); // SS Tower atu_unmap

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accel_ip_pre_boot_config_accel_ip_atu_map_fail_test, nullptr, nullptr)
{
    // Setup
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    // SS Tower success, accel IP atu_map fail for SDMSS
    config_ss_pass();

    will_return(__wrap_atu_map, SILIBS_E_PARAM); // accel ip atu_map

    // SS Tower success, accel IP atu_map fail for CDEDSS
    config_ss_cdedss_pass();

    will_return(__wrap_atu_map, SILIBS_E_PARAM); // accel ip atu_map

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accel_ip_pre_boot_config_accel_ip_atu_unmap_fail_test, nullptr, nullptr)
{
    // Setup
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    // SS Tower success, accel IP atu_unmap fail for SDMSS
    config_ss_pass();

    will_return(__wrap_atu_map, SILIBS_SUCCESS);   // accel ip atu_map
    will_return(__wrap_atu_unmap, SILIBS_E_PARAM); // accel ip atu_unmap

    // SS Tower success, accel IP atu_unmap fail for CDEDSS
    config_ss_cdedss_pass();

    will_return(__wrap_atu_map, SILIBS_SUCCESS);   // accel ip atu_map
    will_return(__wrap_atu_unmap, SILIBS_E_PARAM); // accel ip atu_unmap

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accel_ip_pre_boot_config_wa_hsp_cdedss_tower_init_pass_test, nullptr, nullptr)
{
    // Setup
    will_return(__wrap_idsw_get_die_id, SOC_D0);

    // Happy case for SDMSS
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    will_return(__wrap_atu_map, SILIBS_SUCCESS);   // SS Tower atu_map
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // SS Tower atu_unmap

    config_accel_ip_pass();

    // Happy case for CDEDSS
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    // for SS PCR
    will_return(__wrap_atu_map, SILIBS_SUCCESS);   // SS Tower atu_map
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // SS Tower atu_unmap

    config_accel_ip_cdedss_pass();

    assert_int_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accel_ip_pre_boot_config_wa_hsp_cdedss_tower_init_atu_map_fail_test, nullptr, nullptr)
{
    // Setup
    will_return(__wrap_idsw_get_die_id, SOC_D0);

    // Happy case for SDMSS
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    will_return(__wrap_atu_map, SILIBS_SUCCESS);   // SS Tower atu_map
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // SS Tower atu_unmap

    config_accel_ip_pass();

    // Fail case for CDEDSS
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    // HSP WA INIT - ATU Map FAIL
    will_return(__wrap_atu_map, SILIBS_E_PARAM);

    assert_int_equal(scp_accelerators_init(), ACCEL_RET_FAIL_ATU_MAP);
}

TEST_FUNCTION(accel_ip_pre_boot_config_wa_hsp_cdedss_tower_init_atu_unmap_fail_test, nullptr, nullptr)
{
    // Setup
    will_return(__wrap_idsw_get_die_id, SOC_D0);

    // Happy case for SDMSS
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    will_return(__wrap_atu_map, SILIBS_SUCCESS);   // SS Tower atu_map
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // SS Tower atu_unmap

    config_accel_ip_pass();

    // Fail case for CDEDSS
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    // HSP WA INIT - ATU Unmap FAIL
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, SILIBS_E_PARAM);

    assert_int_equal(scp_accelerators_init(), ACCEL_RET_FAIL_ATU_UNMAP);
}
}
