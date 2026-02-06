//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Mock functions for vab initialization.
 */

/*------------- Includes -----------------*/
#include "pcr_clock_config.h"

#include <FpFwCMocka.h> // IWYU pragma: keep
#include <FpFwUtils.h>
#include <atu_lib.h>
#include <cmocka.h> // IWYU pragma: keep
#include <cper.h>
#include <fpfw_cfg_mgr.h>
#include <idsw.h>
#include <kng_soc_constants.h>
#include <nvic.h>
#include <pcie_ss_common.h>
#include <pciess_int.h>
#include <ras_arm_common.h>
#include <vab_init.h>
#include <vab_intu.h>
#include <vab_utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

idsw_plat_id_t __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(idsw_plat_id_t);
}

nvic_status_t __wrap_nvic_irq_set_isr_with_param(uint32_t irq_num, isr_callback_fn_with_params_t isr, void* parameter)
{
    check_expected(irq_num);
    assert_non_null(isr);
    assert_non_null(parameter);
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

silibs_status_t __wrap_vabss_intu_probe(uintptr_t vab_base, vabss_int_probe_t* probe, INTU_DEST_PIN dest)
{
    check_expected(vab_base);
    assert_non_null(probe);
    check_expected(dest);

    uint8_t intu0_pin = mock();
    bool intu0_value = mock();
    uint8_t intu1_pin = mock();
    bool intu1_value = mock();
    probe->intu0[intu0_pin].asserted = intu0_value;
    probe->intu1[intu1_pin].asserted = intu1_value;

    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_vabss_intu_clear(uintptr_t vab_base, vabss_int_probe_t* probe, INTU_DEST_PIN dest)
{
    check_expected(vab_base);
    assert_non_null(probe);
    check_expected(dest);

    return mock_type(silibs_status_t);
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    check_expected(atu_id);

    atu_map_entry->mscp_start_address = 0xDEADBEEF;

    return mock_type(int);
}

int __wrap_atu_find_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    FPFW_UNUSED(atu_id);
    FPFW_UNUSED(atu_map_entry);

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    check_expected(atu_id);
    FPFW_UNUSED(atu_map_entry);

    return mock_type(int);
}

vab_knobs_t __wrap_config_get_vab_knobs(void)
{
    vab_knobs_t* vab_config = mock_ptr_type(vab_knobs_t*);
    function_called();
    return *vab_config;
}

smmu_vab_prod_knobs_t __wrap_config_get_smmu_vab_knobs(void)
{
    smmu_vab_prod_knobs_t* smmu_vab_config_knob = mock_ptr_type(smmu_vab_prod_knobs_t*);
    function_called();
    return *smmu_vab_config_knob;
}

int __wrap_vab_init(vab_init_t* vab_init_params)
{
    check_expected(vab_init_params->security_state);
    check_expected(vab_init_params->vab_resolved_base_addr);
    check_expected(vab_init_params->vab_configure_intu);
    check_expected(vab_init_params->vab_id);

    function_called();
    return 0;
}

void __wrap_d2d_error_isr(void* context)
{
    FPFW_UNUSED(context);

    function_called();
}

silibs_status_t __wrap_ras_arm_agent_set_base(ras_agent_entity_t* agent, uintptr_t base)
{
    assert_non_null(agent);
    assert_true(base != 0);
    return mock_type(silibs_status_t);
}

bool __wrap_ras_agent_probe(ras_agent_entity_t* agent, ras_error_record_t* record)
{
    assert_non_null(agent);
    assert_non_null(record);

    record->handler = mock_ptr_type(ras_generic_handler_t);
    record->err_type = mock_type(uint64_t);

    return mock_type(bool);
}

void __wrap_ras_print_record(ras_error_record_t* record)
{
    assert_non_null(record);
    function_called();
}

silibs_status_t __wrap_ras_arm_record_to_cper(ras_error_record_t* record, void* cper_record, size_t size)
{
    assert_non_null(record);
    assert_non_null(cper_record);
    assert_true(size > 0);

    return mock_type(silibs_status_t);
}

void __wrap_hm_submit_cper(uint16_t error_domain_idx,
                           acpi_error_severity_t err_severity,
                           acpi_cper_section_t* err_record_section,
                           uint32_t err_record_section_size)
{
    FPFW_UNUSED(err_record_section_size);
    check_expected(error_domain_idx);
    check_expected(err_severity);
    assert_non_null(err_record_section);
    function_called();
}

silibs_status_t __wrap_vab_get_1p_parity_status(uintptr_t vab_base, uint32_t* status)
{
    assert_true(vab_base != 0);
    assert_non_null(status);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_tower_fmu_handler(TOWER_INSTANCE tower_id, DIE_INSTANCE die, uintptr_t base)
{
    FPFW_UNUSED(die);
    FPFW_UNUSED(base);
    assert_true(tower_id < TOWER_MAX_TOWERS_PER_DIE);
    return mock_type(silibs_status_t);
}

pcie_ss_entity_t* __wrap_pciess_get_entity(RPSS_INSTANCE rpss_idx)
{
    FPFW_UNUSED(rpss_idx);
    return mock_ptr_type(pcie_ss_entity_t*);
}

void __wrap_pcie_ss_set_base(pcie_ss_entity_t* ss, uintptr_t base)
{
    assert_non_null(ss);
    assert_true(base != 0);
}

bool __wrap_pciess_probe(pcie_ss_entity_t* ss, pciess_int_probe_t* info, INTU_DEST_PIN dest)
{
    assert_non_null(ss);
    assert_non_null(info);
    assert_true(dest == INTU_TO_SCP);

    return mock_type(bool);
}

void __wrap_rpss_irq_callback(pcie_ss_entity_t* ss, pciess_int_probe_t* info)
{
    assert_non_null(ss);
    assert_non_null(info);
}

silibs_status_t __wrap_pciess_clear_handler(pcie_ss_entity_t* ss, pciess_int_probe_t* info, INTU_DEST_PIN dest)
{
    assert_non_null(ss);
    assert_non_null(info);
    assert_true(dest == INTU_TO_SCP);

    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_vab_fabric_error_trigger_by_type(uintptr_t vab_base, VAB_FABRIC_INTERFACE fabric_if, VAB_FABRIC_ERROR_TYPE type)
{
    FPFW_UNUSED(vab_base);
    FPFW_UNUSED(fabric_if);
    FPFW_UNUSED(type);

    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_vab_ras_trigger_by_type(uintptr_t vab_base,
                                               SUBSYSTEM_WITH_VAB_ID vab_id,
                                               VAB_RAS_NODE_COMPONENT component,
                                               VAB_RAS_NODE_ERROR_TYPE type)
{
    FPFW_UNUSED(vab_base);
    FPFW_UNUSED(vab_id);
    FPFW_UNUSED(component);
    FPFW_UNUSED(type);

    return mock_type(silibs_status_t);
}
