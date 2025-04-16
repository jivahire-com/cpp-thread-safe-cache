//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_mocks.cpp
 * Mock functions for DDR Manager unit tests
 */

/*------------- Includes -----------------*/
#include "ddr_mocks.h"

#include <CMockaWrapper.h>
#include <FpFwUtils.h>
#include <atu_lib.h>
#include <ddrss_lib.h>
#include <ddrss_runtime_api.h>
#include <idsw_kng.h>
#include <silibs_status.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
bool g_should_wrap_idsw_get_platform_sdv = false;

/*------------- Functions ----------------*/

//
// Mocks
//

extern "C" {
uint8_t __wrap_mmio_read8(volatile uint8_t* addr)
{
    FPFW_UNUSED(addr); // todo fixme
    return mock_type(uint8_t);
}

uint16_t __wrap_mmio_read16(volatile uint16_t* addr)
{
    FPFW_UNUSED(addr); // todo fixme
    return mock_type(uint16_t);
}

uint32_t __wrap_mmio_read32(void* addr)
{
    FPFW_UNUSED(addr);
    if (!in_setup_teardown)
    {
        function_called();
        return mock_type(uint32_t);
    }
    else
    {
        return 0;
    }
}

void __wrap_mmio_write8(void* addr, uint8_t data)
{
    if (!in_setup_teardown)
    {
        function_called();
    }

    FPFW_UNUSED(addr);
    FPFW_UNUSED(data);
    return;
}

void __wrap_mmio_write16(volatile uint16_t* addr, uint16_t data)
{
    if (!in_setup_teardown)
    {
        function_called();
    }

    FPFW_UNUSED(addr);
    FPFW_UNUSED(data);
    return;
}

void __wrap_mmio_write32(void* addr, uint32_t data)
{
    if (!in_setup_teardown)
    {
        function_called();
    }

    FPFW_UNUSED(addr);
    FPFW_UNUSED(data);
    return;
}

bool __wrap_ddr_manager_platform_is_polling_supported()
{
    return true;
}

void __wrap_prod_ddrss_lib_init(KNG_DIE_ID die_num)
{
    FPFW_UNUSED(die_num);
    return;
}

bool __wrap_config_get_borgens_1gb_ddr_reserve_enable()
{
    return mock_type(bool);
}

ddrss_phy_training_dq_margin_t* __wrap_ddrss_get_training_margin_base()
{
    return mock_type(ddrss_phy_training_dq_margin_t*);
}

const ddrss_sys_mem_region_t* __wrap_ddrss_get_system_mem_region(void)
{
    return mock_type(const ddrss_sys_mem_region_t*);
}

int32_t __wrap_sds_block_creation(uint32_t sds_module_id, uint32_t request_size, uint32_t region_id)
{
    FPFW_UNUSED(sds_module_id);
    FPFW_UNUSED(request_size);
    FPFW_UNUSED(region_id);

    return mock_type(int32_t);
}

int32_t __wrap_sds_block_write(uint32_t sds_module_id, void* buffer, size_t buffer_size)
{
    FPFW_UNUSED(sds_module_id);
    FPFW_UNUSED(buffer);
    FPFW_UNUSED(buffer_size);

    return mock_type(int32_t);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }

    // Keep mscp base non-zero to allow checking base address in UTs
    atu_map_entry->mscp_start_address = mock_type(uint32_t);

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }

    return mock_type(int);
}

bool __wrap_ddrss_is_valid_local_mc(uint32_t mc)
{
    FPFW_UNUSED(mc);
    return mock_type(bool);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    if (g_should_wrap_idsw_get_platform_sdv)
    {
        return mock_type(KNG_PLAT_ID);
    }
    else
    {
        return PLATFORM_RVP_EVT_SILICON;
    }
}

} // extern "C"
