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
#include <ddr_i3c.h>
#include <ddrss.h>
#include <ddrss_intu.h>
#include <ddrss_lib.h>
#include <ddrss_runtime_api.h>
#include <nvic.h>
#include <sel.h>
#include <silibs_status.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
int __real_ddrss_get_config(ddrss_cfg_knobs_t* ddrss_cfgs);
int __real_ddrss_inject_media_ca_err(uint32_t mc, ddrss_media_ca_err_inj_info_t* ca_err_inj);

// DDR error injection function prototypes
void __real_ddr_err_inj_ecc_ce(uint32_t mc);
void __real_ddr_err_inj_ecc_ue(uint32_t mc);
void __real_ddr_err_inj_mainline_traffic_ce(uint32_t mc);
void __real_ddr_err_inj_mainline_traffic_ue(uint32_t mc);
void __real_ddr_err_inj_media_patrol_scrub_ce(uint32_t mc);
void __real_ddr_err_inj_media_patrol_scrub_ue(uint32_t mc);
void __real_ddr_err_inj_fecq_fedb_data_array_ue(uint32_t mc);
void __real_ddr_err_inj_fedb_merge_data_ue(uint32_t mc);
void __real_ddr_err_inj_fedb_merge_data_ce(uint32_t mc);
void __real_ddr_err_inj_fedb_merge_data_parity_ue(uint32_t mc);
void __real_ddr_err_inj_fedb_merge_strobe_ue(uint32_t mc);
void __real_ddr_err_inj_fedb_merge_strobe_parity_ue(uint32_t mc);
void __real_ddr_err_inj_fedb_strobe_array_ue(uint32_t mc);
void __real_ddr_err_inj_ca_parity_persistent(uint32_t mc);
void __real_ddr_err_inj_ca_parity_transient(uint32_t mc);
void __real_ddr_err_rh_counters_sram_parity(uint32_t mc);
void __real_ddr_err_rh_drfm_sram_parity(uint32_t mc);
void __real_ddr_err_xts_aes_keystore_ce(uint32_t mc);
void __real_ddr_err_xts_aes_keystore_ue(uint32_t mc);
void __real_ddr_err_bcp_read_addr_not_in_ddr(uint32_t mc);
void __real_ddr_err_bcp_read_blocked_by_pas(uint32_t mc);
void __real_ddr_err_bcp_write_addr_not_in_ddr(uint32_t mc);
void __real_ddr_err_bcp_write_blocked_by_pas(uint32_t mc);
void __real_ddr_err_bcp_chi_unsupported_opcode(uint32_t mc);
void __real_ddr_err_inj_mrdp_parity_ue(uint32_t mc);

/*-- Declarations (Statics and globals) --*/
uint32_t g_ddr_intu_sts;
uint32_t g_intu_enable;
uint32_t g_phy_int_sts;
uint32_t g_mc_intu_sts;
uint32_t g_mc_intu_dest_enable;
uint64_t g_ras_err_sts;
bool g_mmio_read32_mocktype;
bool g_should_check_reset_reason_cfg_knobs = false;
bool g_should_check_cper_section = false;
bool g_should_check_ras_agent_entity_id = false;
bool g_should_wrap_ddrss_get_config = false;
bool g_should_wrap_ddrss_inject_media_ca_err = false;
bool g_should_check_ddrss_err_inj_function_ptrs = false;

/*------------- Functions ----------------*/

//
// Mocks
//
void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);
    function_called();
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
    function_called();

    // Make record->reporting_agent non-NULL for later dereferencing
    record->reporting_agent = agent;
    record->status_valid = 1;
    record->status = g_ras_err_sts;

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

int __wrap_ddrss_get_telemetry_record(uint32_t mc, DDRSS_TELEMETRY_TYPE telemetry_type, void* telemetry_buf, int telemetry_buf_len)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(telemetry_type);
    FPFW_UNUSED(telemetry_buf);
    FPFW_UNUSED(telemetry_buf_len);

    return mock_type(int);
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

uintptr_t __wrap_ddrss_atu_map_fips_ns_space(uint32_t die_num)
{
    check_expected(die_num);
    return mock_type(uintptr_t);
}

uintptr_t __wrap_ddrss_atu_map_fips_rt_space(uint32_t die_num)
{
    check_expected(die_num);
    return mock_type(uintptr_t);
}

void __wrap_ddrss_atu_unmap_fips_space(uint32_t die_num)
{
    check_expected(die_num);
    return;
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
    assert_true((error_domain_idx == ACPI_ERROR_DOMAIN_DDR) || (error_domain_idx == ACPI_ERROR_DOMAIN_RHTLM));
    check_expected(err_severity);

    if (g_should_check_cper_section)
    {
        check_expected_ptr(err_record_section);
    }
    FPFW_UNUSED(err_record_section);
    check_expected(err_record_section_size);
}

uint8_t __wrap_config_get_fips_kat_en(void)
{
    return mock_type(uint8_t);
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

    if (status != LED_STATUS_CODE_SCP_DDR_INIT_START)
    {
        assert_in_range(status, LED_STATUS_CODE_SCP_E_DDR0_TRAINING, LED_STATUS_CODE_SCP_E_DDR5_TRAINING);
    }

    function_called();
}

int __wrap_ddrss_get_phy_training_failure(ddrss_phy_training_error_info_t* phy_err_info)
{
    assert_non_null(phy_err_info);
    phy_err_info->mc = mock_type(uint8_t);

    return mock_type(int);
}

void __wrap_ddr_err_inj_ecc_ce(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_ecc_ce(mc);
    }
}

void __wrap_ddr_err_inj_ecc_ue(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_ecc_ue(mc);
    }
}

void __wrap_ddr_err_inj_mainline_traffic_ce(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_mainline_traffic_ce(mc);
    }
}

void __wrap_ddr_err_inj_mainline_traffic_ue(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_mainline_traffic_ue(mc);
    }
}

void __wrap_ddr_err_inj_media_patrol_scrub_ce(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_media_patrol_scrub_ce(mc);
    }
}

void __wrap_ddr_err_inj_media_patrol_scrub_ue(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_media_patrol_scrub_ue(mc);
    }
}

void __wrap_ddr_err_inj_fecq_fedb_data_array_ue(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_fecq_fedb_data_array_ue(mc);
    }
}

void __wrap_ddr_err_inj_fedb_merge_data_ue(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_fedb_merge_data_ue(mc);
    }
}

void __wrap_ddr_err_inj_fedb_merge_data_ce(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_fedb_merge_data_ce(mc);
    }
}

void __wrap_ddr_err_inj_fedb_merge_data_parity_ue(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_fedb_merge_data_parity_ue(mc);
    }
}

void __wrap_ddr_err_inj_fedb_merge_strobe_ue(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_fedb_merge_strobe_ue(mc);
    }
}

void __wrap_ddr_err_inj_fedb_merge_strobe_parity_ue(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_fedb_merge_strobe_parity_ue(mc);
    }
}

void __wrap_ddr_err_inj_fedb_strobe_array_ue(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_fedb_strobe_array_ue(mc);
    }
}

void __wrap_ddr_err_inj_ca_parity_persistent(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_ca_parity_persistent(mc);
    }
}

void __wrap_ddr_err_inj_ca_parity_transient(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_ca_parity_transient(mc);
    }
}

void __wrap_ddr_err_rh_counters_sram_parity(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_rh_counters_sram_parity(mc);
    }
}

void __wrap_ddr_err_rh_drfm_sram_parity(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_rh_drfm_sram_parity(mc);
    }
}

void __wrap_ddr_err_xts_aes_keystore_ce(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_xts_aes_keystore_ce(mc);
    }
}

void __wrap_ddr_err_xts_aes_keystore_ue(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_xts_aes_keystore_ue(mc);
    }
}

void __wrap_ddr_err_bcp_read_addr_not_in_ddr(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_bcp_read_addr_not_in_ddr(mc);
    }
}

void __wrap_ddr_err_bcp_read_blocked_by_pas(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_bcp_read_blocked_by_pas(mc);
    }
}

void __wrap_ddr_err_bcp_write_addr_not_in_ddr(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_bcp_write_addr_not_in_ddr(mc);
    }
}

void __wrap_ddr_err_bcp_write_blocked_by_pas(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_bcp_write_blocked_by_pas(mc);
    }
}

void __wrap_ddr_err_bcp_chi_unsupported_opcode(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_bcp_chi_unsupported_opcode(mc);
    }
}

void __wrap_ddr_err_inj_mrdp_parity_ue(uint32_t mc)
{
    FPFW_UNUSED(mc);
    if (g_should_check_ddrss_err_inj_function_ptrs)
    {
        function_called();
    }
    else
    {
        __real_ddr_err_inj_mrdp_parity_ue(mc);
    }
}

// New mocks for ddrss_update_ppr_completion
int __wrap_ddr_i3c_interface_read_spd_nvm_data(i3c_cmd_t* i3c_cmd,
                                               uint8_t dimm,
                                               uint8_t* read_data,
                                               uint16_t* data_len,
                                               uint16_t* data_offset,
                                               uint16_t read_len)
{
    FPFW_UNUSED(i3c_cmd);
    FPFW_UNUSED(dimm);
    FPFW_UNUSED(read_len);
    // Touch data_offset so compiler doesn't warn
    if (data_offset != NULL)
    {
        uint16_t off_tmp = *data_offset;
        FPFW_UNUSED(off_tmp);
    }
    // Simulate successful read; test will set expectations by will_return/mock_type sequence
    // Provide deterministic buffer contents so test can validate modifications when needed.
    for (uint8_t i = 0; i < SIZE_16_BYTES; i++)
    {
        read_data[i] = (uint8_t)i; // seed pattern
    }
    *data_len = SIZE_16_BYTES;
    // data_offset passed in contains starting offset; leave unchanged
    return mock_type(int);
}

int __wrap_ddr_i3c_interface_write_spd_nvm_data(i3c_cmd_t* i3c_cmd,
                                                uint8_t dimm,
                                                uint8_t* write_data,
                                                uint16_t* data_len,
                                                uint16_t* UNUSED_data_offset,
                                                uint16_t write_len)
{
    FPFW_UNUSED(i3c_cmd);
    FPFW_UNUSED(dimm);
    // Touch data_offset to suppress unused warning even when FPFW_UNUSED treated as strict
    if (UNUSED_data_offset != NULL)
    {
        uint16_t tmp_off = *UNUSED_data_offset;
        FPFW_UNUSED(tmp_off);
    }
    FPFW_UNUSED(write_len);
    // Capture what is being written for verification using check_expected_memory if desired.
    check_expected_ptr(write_data);
    // Assume successful write; length echoes back write size
    *data_len = write_len;
    return mock_type(int);
}

int __wrap_log_sel_event(sel_event_record_t* sel_event)
{
    assert_non_null(sel_event);
    // Allow test validation of key fields
    check_expected(sel_event->ddr_info.record_type);
    check_expected(sel_event->ddr_info.oem_data_1);
    check_expected(sel_event->ddr_info.oem_data_2);
    return mock_type(int);
}

// Vendor ID helper mock
uint8_t __wrap_get_dimm_vendor_id(uint16_t dimm_local_idx)
{
    // Provide path control for vendor id; tests will will_return desired value
    FPFW_UNUSED(dimm_local_idx);
    return mock_type(uint8_t);
}
