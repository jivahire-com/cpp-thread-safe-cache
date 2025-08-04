//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddrss_mocks.c
 * Mock functions for DDRSS unit tests
 * @file ddrss_mocks.c
 * Mock functions for DDRSS unit tests
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // IWYU pragma: keep
#include <FpFwUtils.h>
#include <atu_lib.h>
#include <boot_status.h>
#include <cmn800.h>
#include <cmocka.h> // IWYU pragma: keep
#include <ddrss.h>
#include <ddrss_intu.h>
#include <ddrss_lib.h>
#include <nvic.h>
#include <silibs_status.h>
#include <stddef.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
int __real_ddrss_get_config(ddrss_cfg_knobs_t* ddrss_cfgs);
int __real_ddrss_inject_media_ca_err(uint32_t mc, ddrss_media_ca_err_inj_info_t* ca_err_inj);

/*-- Declarations (Statics and globals) --*/
uint32_t g_ddr_intu_sts;
uint32_t g_intu_enable;
uint32_t g_phy_int_sts;
uint32_t g_mc_intu_sts;
uint32_t g_mc_intu_dest_enable;
bool g_mmio_read32_mocktype;
bool g_should_check_reset_reason_cfg_knobs = false;
bool g_should_check_cper_section = false;
bool g_should_check_ras_agent_entity_id = false;
bool g_should_wrap_ddrss_get_config = false;
bool g_should_wrap_ddrss_inject_media_ca_err = false;

/*------------- Functions ----------------*/

//
// Mocks
//
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

int __wrap_ddrss_init(ddrss_cfg_knobs_t* cfg_knobs)
{
    if (g_should_check_reset_reason_cfg_knobs)
    {
        check_expected(cfg_knobs->reset_reason);
    }
    return mock_type(int);
}

nvic_status_t __wrap_nvic_irq_set_isr_with_param(uint32_t irq_num, isr_callback_fn_with_params_t isr, void* parameter)
{
    uint32_t ddrss_num;
    ddrss_num = *(uint32_t*)parameter;

    check_expected(irq_num);
    check_expected(isr);
    check_expected(ddrss_num);
    return (NVIC_STATUS_SUCCESS);
}

nvic_status_t __wrap_nvic_irq_clear_pending(uint32_t irq_num)
{
    check_expected(irq_num);
    return (NVIC_STATUS_SUCCESS);
}

nvic_status_t __wrap_nvic_irq_enable(uint32_t irq_num)
{
    check_expected(irq_num);
    return (NVIC_STATUS_SUCCESS);
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    FPFW_UNUSED(addr);
    if (g_mmio_read32_mocktype)
    {
        return (mock_type(uint32_t));
    }
    else
    {
        return (0);
    }
}

void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    FPFW_UNUSED(addr);
    FPFW_UNUSED(data);
    return;
}

// This is the crux of the DDR ISR testing - it will set which interrupt is 'triggered'
int __wrap_intu_get_interrupt_status(uintptr_t intu_base_addr, uint32_t* intr)
{
    FPFW_UNUSED(intu_base_addr);
    *intr = g_ddr_intu_sts;
    return (SILIBS_SUCCESS);
}

int __wrap_ddrss_ddr_intu_get_interrupt_dest_enable(uint32_t mc, DDRSS_INTU_OUT dest_pin, uint32_t* enable)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(dest_pin);
    *enable = g_intu_enable;
    return (SILIBS_SUCCESS);
}

int __wrap_ddrss_probe_ras_agent(uint32_t mc, uint32_t ras_agent_entity_id)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(ras_agent_entity_id);
    return (mock_type(int));
}

int __wrap_ddrss_get_phy_interrupt_status(uint32_t mc, uint32_t* phy_int_sts)
{
    FPFW_UNUSED(mc);
    *phy_int_sts = g_phy_int_sts;
    return SILIBS_SUCCESS;
}

int __wrap_ddrss_clear_phy_interrupt_status(uint32_t mc, uint32_t phy_int_sts)
{
    FPFW_UNUSED(mc);
    check_expected(phy_int_sts);
    return SILIBS_SUCCESS;
}

int __wrap_ddrss_mc_intu_get_interrupt_dest_enable(uint32_t mc, DDRSS_INTU_OUT dest_pin, uint32_t* enable)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(dest_pin);
    *enable = g_mc_intu_dest_enable;
    return SILIBS_SUCCESS;
}

int __wrap_ddrss_mc_event_clear_interrupt(uint32_t mc, uint32_t intr_evt_mask)
{
    FPFW_UNUSED(mc);
    check_expected(intr_evt_mask);
    return SILIBS_SUCCESS;
}

int __wrap_ddrss_ddr_intu_clear_destination_interrupt(uint32_t mc, DDRSS_INTU_OUT dest_pin)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(dest_pin);
    return SILIBS_SUCCESS;
}

int __wrap_ddrss_ddr_intu_clear_interrupt(uint32_t mc, uint32_t intr_mask)
{
    FPFW_UNUSED(mc);
    check_expected(intr_mask);
    return SILIBS_SUCCESS;
}

bool __wrap_ras_arm_agent_probe(ras_agent_entity_t* agent, ras_error_record_t* record)
{
    FPFW_UNUSED(agent);
    FPFW_UNUSED(record);
    FPFW_UNUSED(record);
    function_called();
    return SILIBS_SUCCESS;
}
int __wrap_ddrss_convert_ras_rec_to_cper(uint32_t mc,
                                         ras_error_record_t* record,
                                         acpi_err_sec_memory_t* ddr_ras_cper,
                                         acpi_err_sec_mem_vendor_t* ddr_vendor_cper)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(record);
    FPFW_UNUSED(ddr_ras_cper);
    FPFW_UNUSED(ddr_vendor_cper);
    function_called();
    return SILIBS_SUCCESS;
}
int __wrap_ras_print_record(ras_error_record_t* record)
{
    FPFW_UNUSED(record);
    function_called();
    return SILIBS_SUCCESS;
}
int __wrap_ddrss_get_ras_agent(uint32_t mc, DDRSS_RAS_NODE_ID ras_agent_entity_id, ras_agent_entity_t** ras_agent)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(ras_agent_entity_id);
    FPFW_UNUSED(ras_agent);
    if (g_should_check_ras_agent_entity_id)
    {
        check_expected(ras_agent_entity_id);
        *ras_agent = mock_type(ras_agent_entity_t*);
    }

    return mock_type(int);
}

int __wrap_ddrss_mc_intu_get_interrupt_status(uint32_t mc, uint32_t* intr)
{
    FPFW_UNUSED(mc);
    *intr = g_mc_intu_sts;
    return (SILIBS_SUCCESS);
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

cmn800_snf_to_mc_config_t* __wrap_cmn800_generate_ddr_mc_map_from_cached_config(void)
{
    return mock_type(cmn800_snf_to_mc_config_t*);
}

uintptr_t __wrap_ddrss_atu_map_cfg_space(uint32_t die_num)
{
    check_expected(die_num);
    return mock_type(uintptr_t);
}

void __wrap_ddrss_atu_unmap_cfg_space(uint32_t die_num)
{
    check_expected(die_num);
}

int __wrap_ddrss_physical_to_media_addr(uint64_t pa, ddrss_media_addr_t* ma, uint32_t* mc)
{
    FPFW_UNUSED(pa);
    FPFW_UNUSED(ma);
    FPFW_UNUSED(mc);
    return (SILIBS_SUCCESS);
}

bool __wrap_system_info_is_warm_start(void)
{
    return mock_type(bool);
}

void __wrap_hm_submit_cper(uint16_t error_domain_idx,
                           acpi_error_severity_t err_severity,
                           acpi_cper_section_t* err_record_section,
                           uint32_t err_record_section_size)
{
    assert_int_equal(error_domain_idx, ACPI_ERROR_DOMAIN_DDR);
    check_expected(err_severity);

    if (g_should_check_cper_section)
    {
        check_expected_ptr(err_record_section);
    }
    FPFW_UNUSED(err_record_section);
    check_expected(err_record_section_size);
}

int __wrap_ddrss_get_config(ddrss_cfg_knobs_t* ddrss_cfgs)
{
    if (g_should_wrap_ddrss_get_config)
    {
        PLATFORM_MEMCPY(ddrss_cfgs, mock_type(ddrss_cfg_knobs_t*), sizeof(ddrss_cfg_knobs_t));
        return SILIBS_SUCCESS;
    }

    return __real_ddrss_get_config(ddrss_cfgs);
}

int __wrap_ddrss_inject_media_data_err(uint32_t mc, const ddrss_media_data_err_inj_info_t* media_err_inj)
{
    check_expected(mc);
    check_expected(media_err_inj->err_rs_sym);
    check_expected(media_err_inj->err_inj_rw);
    check_expected(media_err_inj->err_inj_beat);
    check_expected(media_err_inj->err_inj_cnt);
    return 0;
}

int __wrap_ddrss_inject_media_ca_err(uint32_t mc, ddrss_media_ca_err_inj_info_t* ca_err_inj)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(ca_err_inj);
    if (g_should_wrap_ddrss_inject_media_ca_err)
    {
        function_called();
        return SILIBS_SUCCESS;
    }

    return __real_ddrss_inject_media_ca_err(mc, ca_err_inj);
}

void __wrap_FpFwAssert(int expression)
{
    if (!expression)
    {
        check_expected(expression);
    }
}

void __wrap_post_led_status(boot_status_req_t* p_req_mem, led_status_codes_t status)
{
    assert_non_null(p_req_mem);
    assert_in_range(status, LED_STATUS_CODE_SCP_E_DDR0_TRAINING, LED_STATUS_CODE_SCP_E_DDR5_TRAINING);

    function_called();
}

int __wrap_ddrss_get_phy_training_failure(ddrss_phy_training_error_info_t* phy_err_info)
{
    assert_non_null(phy_err_info);
    phy_err_info->mc = mock_type(uint8_t);

    return mock_type(int);
}
