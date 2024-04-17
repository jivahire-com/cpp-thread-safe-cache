//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_test.cpp
 * Unit test file for accelerator_ip_test lib
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h> // for TEST_FUNCTION, assert_int_equal
#include <cstddef>         // for NULL

extern "C" {
#include <accelerator_ip.h>
#include <idsw.h>
#include <utils.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
void __wrap_configure_sdmss_system_addr_map(uintptr_t tower_base_addr, SDMSS_INSTANCE sdmss_id)
{
    UNUSED(tower_base_addr);
    UNUSED(sdmss_id);
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

DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(DIE_ID);
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

int __wrap_smmu_enable_access_check(uintptr_t smmu_base_addr)
{
    smmu_base_addr = mock_type(uintptr_t);

    if (smmu_base_addr == 0x0)
    {
        return SILIBS_E_PARAM;
    }

    return SILIBS_SUCCESS;
}

TEST_FUNCTION(accel_ip_pre_boot_config_pass_test, nullptr, nullptr)
{
    // Happy case
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);              // VAB init atu_map
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);            // VAB init atu_unmap
    will_return(__wrap_smmu_enable_access_check, 0x12345678); // smmu_enable_access_check
    will_return(__wrap_atu_map, SILIBS_SUCCESS);              // SS Tower init atu_map
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);            // SS Tower init atu_unmap
    will_return(__wrap_atu_map, SILIBS_SUCCESS);              // configure_accel_ip atu_map
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);            // configure_accel_ip atu_unmap
    assert_int_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accel_ip_pre_boot_config_fail_test, nullptr, nullptr)
{
    // VAB atu_map fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return(__wrap_atu_map, SILIBS_E_PARAM);
    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);

    // VAB atu_map pass, VAB atu_unmap fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS); // VAB atu_map
    will_return(__wrap_smmu_enable_access_check, 0x0);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // VAB atu_unmap
    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);

    // VAB atu_map success, smmu_enable_access_check success, VAB atu_unmap fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS); // VAB atu_map
    will_return(__wrap_smmu_enable_access_check, 0x12345678);
    will_return(__wrap_atu_unmap, SILIBS_E_PARAM); // VAB atu_unmap
    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);

    // VAB init success, SS Tower atu_map fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS); // VAB atu_map
    will_return(__wrap_smmu_enable_access_check, 0x12345678);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // VAB atu_unmap
    will_return(__wrap_atu_map, SILIBS_E_PARAM);   // SS Tower atu_map
    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);

    // VAB init success, SS Tower atu_map success, SS Tower atu_unmap fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS); // VAB atu_map
    will_return(__wrap_smmu_enable_access_check, 0x12345678);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // VAB atu_unmap
    will_return(__wrap_atu_map, SILIBS_SUCCESS);   // SS Tower atu_map
    will_return(__wrap_atu_unmap, SILIBS_E_PARAM); // SS Tower atu_unmap
    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);

    // VAB init success, SS Tower success, accel IP atu_map fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS); // VAB atu_map
    will_return(__wrap_smmu_enable_access_check, 0x12345678);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // VAB atu_unmap
    will_return(__wrap_atu_map, SILIBS_SUCCESS);   // SS Tower atu_map
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // SS Tower atu_unmap
    will_return(__wrap_atu_map, SILIBS_E_PARAM);   // accel ip atu_map
    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);

    // VAB init success, SS Tower success, accel IP atu_map fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS); // VAB atu_map
    will_return(__wrap_smmu_enable_access_check, 0x12345678);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // VAB atu_unmap
    will_return(__wrap_atu_map, SILIBS_SUCCESS);   // SS Tower atu_map
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS); // SS Tower atu_unmap
    will_return(__wrap_atu_map, SILIBS_SUCCESS);   // accel ip atu_map
    will_return(__wrap_atu_unmap, SILIBS_E_PARAM); // accel ip atu_unmap
    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}
}