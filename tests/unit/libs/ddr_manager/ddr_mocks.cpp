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
#include <fpfw_icc_base.h>
#include <idsw_kng.h>
#include <silibs_status.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <variable_services.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
bool g_should_wrap_idsw_get_platform_sdv = false;
bool g_should_wrap_ddr_create_memory_map = false;
bool g_should_wrap_ddrss_get_ddrss_mask = false;
bool g_check_smb17_params = false;

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

void __wrap_prod_ddrss_lib_init(KNG_DIE_ID die_num)
{
    FPFW_UNUSED(die_num);
    return;
}

bool __wrap_config_get_ddrmanager_bwl_polling_en()
{
    return mock_type(bool);
}

bool __wrap_config_get_ras_init_en()
{
    return mock_type(bool);
}

bool __wrap_config_get_borgens_1gb_ddr_reserve_enable()
{
    return mock_type(bool);
}

uint64_t __wrap_config_get_rh_tlm_service_period_ms(void)
{
    return mock_type(uint64_t);
}

bool __wrap_config_get_erhm_en()
{
    return mock_type(bool);
}

bool __wrap_config_get_ddrmanager_sdl_en()
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

int __wrap_prod_ddrss_get_intr_event_cper(uint32_t mc, uint32_t intr_event, acpi_err_sec_mem_vendor_t* ddr_cper)
{
    check_expected(mc);
    check_expected(intr_event);
    assert_true(ddr_cper != NULL);

    return mock_type(int);
}

void __wrap_hm_submit_cper(uint16_t error_domain_idx, acpi_error_severity_t err_severity, void* err_record_section, uint32_t err_record_section_size)
{
    check_expected(error_domain_idx);
    check_expected(err_severity);
    assert_true(err_record_section != NULL);
    assert_true(err_record_section_size > 0);

    function_called();
}

unsigned int __wrap__tx_thread_sleep(unsigned long ticks)
{
    FPFW_UNUSED(ticks);
    return 0;
}

int __wrap_ddrss_bandwidth_limiter_config(uint32_t mc, bool enable, uint32_t max_acc_cost, uint32_t rd_wr_cost)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(enable);
    FPFW_UNUSED(max_acc_cost);
    FPFW_UNUSED(rd_wr_cost);
    return mock_type(int);
}

void __wrap_ddr_create_memory_map()
{
    if (g_should_wrap_ddr_create_memory_map)
    {
        function_called();
    }
    else
    {
        __real_ddr_create_memory_map();
    }
    return;
}

uint16_t __wrap_ddrss_get_ddrss_mask(void)
{
    if (g_should_wrap_ddrss_get_ddrss_mask)
    {
        return mock_type(uint16_t);
    }
    else
    {
        return __real_ddrss_get_ddrss_mask();
    }
}

int __wrap_prod_ddrss_get_ras_erg_ce_interrupt_enable(uint32_t mc, DDRSS_RAS_NODE_ID erg_id, bool* enable)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(erg_id);
    assert_true(enable != NULL);

    *enable = mock_type(bool);
    return mock_type(int);
}

int __wrap_prod_ddrss_set_ras_erg_ce_interrupt_enable(uint32_t mc, DDRSS_RAS_NODE_ID erg_id, bool enable)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(erg_id);
    check_expected(enable);

    return mock_type(int);
}

bool __wrap_FPFwCoreInterruptIsEnabled(uint32_t irqnum)
{
    check_expected(irqnum);
    return mock_type(bool);
}

uint32_t __wrap_FPFwCoreInterruptDisableVector(uint32_t irqnum)
{
    check_expected(irqnum);
    return mock_type(uint32_t);
}

uint32_t __wrap_FPFwCoreInterruptEnableVector(uint32_t irqnum)
{
    check_expected(irqnum);
    return mock_type(uint32_t);
}

uint32_t __wrap_gtimer_prodfw_get_frequency(void)
{
    return mock_type(uint32_t);
}

void __wrap_copy_single_smbios_type_17(uint8_t* dest_addr, SMBIOS_MEM_DEVICE_17* smb_table17)
{
    FPFW_UNUSED(dest_addr);
    FPFW_UNUSED(smb_table17)

    if (g_check_smb17_params)
    {
        // Check each parameter of the SMBIOS Type 17 structure
        check_expected(smb_table17->Type);
        check_expected(smb_table17->Length);
        check_expected(smb_table17->Handle);
        check_expected(smb_table17->PhysMemoryArrayHandle);
        check_expected(smb_table17->MemoryErrorInfoHandle);
        check_expected(smb_table17->TotalWidth);
        check_expected(smb_table17->DataWidth);
        check_expected(smb_table17->FormFactor);
        check_expected(smb_table17->DeviceSet);
        check_expected(smb_table17->MemoryType);
        check_expected(smb_table17->TypeDetail);
        check_expected(smb_table17->Attributes); // This is Rank
        check_expected(smb_table17->ExtendedSize);
        check_expected(smb_table17->MinVoltage);
        check_expected(smb_table17->MaxVoltage);
        check_expected(smb_table17->ConfiguredVoltage);
        check_expected(smb_table17->MemoryTechnology);
        check_expected(smb_table17->MemoryOperatingModeCapability);
        check_expected(smb_table17->MemorySubsystemControllerManufacturerID);
        check_expected(smb_table17->MemorySubsystemControllerProductID);
        check_expected(smb_table17->NonvolatileSize);
        check_expected(smb_table17->CacheSize);
        check_expected(smb_table17->LogicalSize);
        check_expected(smb_table17->ExtendedSpeed);
        check_expected(smb_table17->ExtendedConfiguredMemorySpeed);
    }

    return;
}

int __wrap_ddrss_get_telemetry_record(uint32_t mc, DDRSS_TELEMETRY_TYPE telemetry_type, void* telemetry_buf, int telemetry_buf_len)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(telemetry_type);
    FPFW_UNUSED(telemetry_buf);
    FPFW_UNUSED(telemetry_buf_len);

    return mock_type(int);
}

void __wrap_prod_ddrss_convert_rh_cfg_rec_to_rh_cper(uint32_t mc, void* rh_cfg_rec, void* rh_cper)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(rh_cfg_rec);
    FPFW_UNUSED(rh_cper);

    function_called();
}

int32_t __wrap_variable_service_initialize_ctx(var_service_req_ctx_t* var_serv_ctx, var_service_shared_mem_t* mem_ctx)
{
    FPFW_UNUSED(var_serv_ctx);
    FPFW_UNUSED(mem_ctx);

    var_serv_ctx->is_initialized = true;

    return mock_type(int32_t);
}

int32_t __wrap_variable_service_sync_get_variable(var_service_req_ctx_t* var_serv_ctx, var_service_req_params_t* req_params)
{
    FPFW_UNUSED(var_serv_ctx);
    FPFW_UNUSED(req_params);

    return mock_type(int32_t);
}

void __wrap_variable_service_unlock_get_var_ctx(var_service_req_ctx_t* var_serv_ctx)
{
    FPFW_UNUSED(var_serv_ctx);

    var_serv_ctx->is_initialized = false;

    return;
}

void __wrap_variable_service_sync_set_variable(var_service_req_ctx_t* var_serv_ctx, var_service_req_params_t* req_params)
{
    FPFW_UNUSED(var_serv_ctx);
    FPFW_UNUSED(req_params);

    return;
}

fpfw_icc_base_ctx_t* __wrap_get_icc_base_ctx(void)
{
    return mock_type(fpfw_icc_base_ctx_t*);
}

void __wrap_sdl_get_mem_ctx(var_service_shared_mem_t* mem_ctx)
{
    mem_ctx->payload_base = mock_type(uintptr_t);
    mem_ctx->max_payload_size = mock_type(size_t);

    return;
}

} // extern "C"
