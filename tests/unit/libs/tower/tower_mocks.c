//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tower_mocks.c
 * Mock functions for tower sequence
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // IWYU pragma: keep
#include <FpFwUtils.h>
#include <accelerator_ip.h>
#include <atu_lib.h>
#include <cmocka.h> // IWYU pragma: keep
#include <cper.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h>
#include <hsp_firmware_headers.h> // for HSP_MAILBOX_CMD_BOOT_STATUS_NOTIFY, HspFirmwareIdScp...
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h>
#include <ras_agent.h>
#include <ras_common.h>
#include <silibs_status.h>
#include <stddef.h>
#include <tower_fmu_utility.h>
#include <tower_sequence.h>
#include <tower_utility.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static bool g_ras_agent_probe_misc_0_valid;
static uint32_t g_ras_agent_probe_misc_0;
static uint32_t g_ras_agent_probe_err_type;
static bool g_ras_agent_probe_fields_override_set;

/*------------- Functions ----------------*/

void tower_mocks_set_ras_agent_probe_fields(bool misc_0_valid, uint32_t misc_0, uint32_t err_type)
{
    g_ras_agent_probe_misc_0_valid = misc_0_valid;
    g_ras_agent_probe_misc_0 = misc_0;
    g_ras_agent_probe_err_type = err_type;
    g_ras_agent_probe_fields_override_set = true;
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }
    // Keep mscp base non-zero to allow checking base address in UTs
    atu_map_entry->mscp_start_address = 0xffffffff;

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

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

tower_knobs_t __wrap_config_get_tower_knobs(void)
{
    tower_knobs_t* tower_config = mock_ptr_type(tower_knobs_t*);
    function_called();
    return *tower_config;
}

int __wrap_tower_sequence_configure_towers(tower_sequence_soc_init_params_t* tower_sequence_param)
{
    check_expected(tower_sequence_param->tower_configure_fabric_apu);
    check_expected(tower_sequence_param->tower_configure_fabric_fmu);
    check_expected(tower_sequence_param->tower_fabric_tower_resolved_addr);

    check_expected(tower_sequence_param->tower_configure_periph_apu);
    check_expected(tower_sequence_param->tower_configure_periph_fmu);
    check_expected(tower_sequence_param->tower_periph_tower_resolved_addr);

    check_expected(tower_sequence_param->tower_configure_d2dss_apu);
    check_expected(tower_sequence_param->tower_configure_d2dss_fmu);
    check_expected(tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr);
    check_expected(tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr);

    check_expected(tower_sequence_param->tower_configure_vab_sam);
    check_expected(tower_sequence_param->tower_configure_vab_apu);
    check_expected(tower_sequence_param->tower_configure_vab_fmu);
    check_expected(tower_sequence_param->tower_vab_instances_enabled);

    check_expected(tower_sequence_param->tower_configure_rpss_sam);
    check_expected(tower_sequence_param->tower_configure_rpss_apu);
    check_expected(tower_sequence_param->tower_configure_rpss_fmu);
    check_expected(tower_sequence_param->tower_rpss_instances_enabled);

    check_expected(tower_sequence_param->tower_configure_sdmss_sam);
    check_expected(tower_sequence_param->tower_configure_sdmss_apu);
    check_expected(tower_sequence_param->tower_configure_sdmss_fmu);
    check_expected(tower_sequence_param->tower_sdmss_isolation_enabled);
    check_expected(tower_sequence_param->tower_sdmss_tower_resolved_addr);

    check_expected(tower_sequence_param->tower_configure_ioss_sam);
    check_expected(tower_sequence_param->tower_configure_ioss_apu);
    check_expected(tower_sequence_param->tower_configure_ioss_fmu);
    check_expected(tower_sequence_param->tower_ioss_tower_resolved_addr);

    check_expected(tower_sequence_param->die_id);
    function_called();
    return 0;
}

fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size, size_t* output_recv_bytes)
{
    check_expected_ptr(icc_ctx);
    check_expected_ptr(payload_buffer);
    check_expected(buffer_size);
    check_expected_ptr(output_recv_bytes);

    kng_hsp_mailbox_msg* recv_msg = (kng_hsp_mailbox_msg*)payload_buffer;
    recv_msg->header.cmd = HSP_MAILBOX_CMD_POST_SCP_INIT_TOWER_CONFIG_RSP;
    recv_msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
    *output_recv_bytes = sizeof(kng_hsp_mailbox_msg);

    return mock_type(fpfw_status_t);
}

bool __wrap_system_info_is_hsp_present()
{
    return mock_type(bool);
}

bool __wrap_accel_is_isolation_enabled(ACCEL_ID acc_id)
{
    check_expected(acc_id);

    return mock_type(bool);
}

ras_agent_entity_t* __wrap_tower_get_ras_agent_entity(uint32_t tower_id)
{
    assert_true(tower_id < TOWER_MAX_TOWERS_PER_DIE);
    return mock_ptr_type(ras_agent_entity_t*);
}

silibs_status_t __wrap_ras_arm_fmu_agent_set_base(ras_agent_entity_t* agent, uintptr_t base, TOWER_INSTANCE tower_id)
{
    assert_non_null(agent);
    assert_true(tower_id < TOWER_MAX_TOWERS_PER_DIE);
    FPFW_UNUSED(base);

    return mock_type(silibs_status_t);
}

bool __wrap_ras_agent_probe(ras_agent_entity_t* agent, ras_error_record_t* record)
{
    assert_non_null(agent);
    assert_non_null(record);

    if (g_ras_agent_probe_fields_override_set)
    {
        record->misc_0_valid = g_ras_agent_probe_misc_0_valid;
        record->misc_0 = g_ras_agent_probe_misc_0;
        record->err_type = g_ras_agent_probe_err_type;
        g_ras_agent_probe_fields_override_set = false;
    }
    record->handler = mock_ptr_type(ras_generic_handler_t);
    record->err_code_valid = mock_type(bool);
    record->err_code = mock_type(uint32_t);

    return mock_type(bool);
}

void __wrap_ras_print_record(ras_error_record_t* record)
{
    assert_non_null(record);
    function_called();
}

silibs_status_t __wrap_ras_agent_record_to_cper(ras_error_record_t* record, void* cper, size_t cper_size, uint32_t* severity)
{
    assert_non_null(record);
    assert_non_null(cper);
    assert_true(cper_size > 0);

    *severity = mock_type(uint32_t);
    return mock_type(silibs_status_t);
}

void __wrap_hm_submit_cper(uint16_t error_domain_idx,
                           acpi_error_severity_t err_severity,
                           acpi_cper_section_t* err_record_section,
                           uint32_t err_record_section_size)
{
    FPFW_UNUSED(error_domain_idx);
    FPFW_UNUSED(err_severity);
    FPFW_UNUSED(err_record_section_size);
    assert_non_null(err_record_section);
}

silibs_status_t __wrap_tower_fmu_inject_single_error(uintptr_t tower_base_addr,
                                                     TOWER_INSTANCE tower_id,
                                                     tower_node_entity_t tower_node,
                                                     TOWER_ERR_PROTECTION_TYPE err)
{
    FPFW_UNUSED(tower_base_addr);
    FPFW_UNUSED(tower_id);
    FPFW_UNUSED(tower_node);
    FPFW_UNUSED(err);

    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_ras_arm_fmu_agent_trigger_by_type(ras_agent_entity_t* agent, uint64_t types)
{
    assert_non_null(agent);
    assert_true(types != 0);

    return mock_type(silibs_status_t);
}
