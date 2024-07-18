//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Mock functions for vab initialization.
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // IWYU pragma: keep
#include <FpFwUtils.h>
#include <atu_lib.h>
#include <cmocka.h> // IWYU pragma: keep
#include <idsw.h>
#include <nvic.h>
#include <vab_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
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

int __wrap_intu_clear_interrupt_status(uintptr_t intu_base_addr, uint32_t intr_pin_mask)
{
    check_expected(intu_base_addr);
    check_expected(intr_pin_mask);
    return 0;
}

int __wrap_intu_get_interrupt_status(uintptr_t intu_base_addr, uint32_t* intr)
{
    check_expected(intu_base_addr);
    assert_non_null(intr);
    return 0;
}

void __wrap_rpss_irq_callback(uint32_t irq_num)
{
    check_expected(irq_num);
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    check_expected(atu_id);

    /* Keep mscp base zero to allow checking base address in UTs */
    atu_map_entry->mscp_start_address = 0x0;

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    check_expected(atu_id);
    FPFW_UNUSED(atu_map_entry);

    return mock_type(int);
}

int __wrap_vab_init(vab_init_t* vab_init_params)
{
    check_expected(vab_init_params->security_state);
    check_expected(vab_init_params->vab_smmu_gbpa_cfg->sh_cfg);
    check_expected(vab_init_params->system_counter_delay);
    check_expected(vab_init_params->vab_resolved_base_addr);
    check_expected(vab_init_params->vab_configure_intu);

    function_called();
    return 0;
}
