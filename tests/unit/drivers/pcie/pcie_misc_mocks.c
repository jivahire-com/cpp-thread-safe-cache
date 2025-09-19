//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_misc_mocks.c
 * Misc. mock functions used by PCIe driver unit tests
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // IWYU pragma: keep
#include <FpFwUtils.h>
#include <cmocka.h> // IWYU pragma: keep
#include <cper.h>
#include <fpfw_cfg_mgr.h>
#include <kng_soc_constants.h>
#include <pcie_ss_common.h> // for pcie_ss_entity_t, ss_bases_t
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void __wrap_enable_vab_isrs(uint16_t vab_instances_to_init)
{
    check_expected(vab_instances_to_init);
}

cxl_region_params_t __wrap_config_get_cxl_params_die0(void)
{
    cxl_region_params_t* params = mock_ptr_type(cxl_region_params_t*);
    return *params;
}

cxl_region_params_t __wrap_config_get_cxl_params_die1(void)
{
    cxl_region_params_t* params = mock_ptr_type(cxl_region_params_t*);
    return *params;
}

uint8_t __wrap_system_info_get_board_id()
{
    return mock_type(uint8_t);
}

bool __wrap_config_get_pcie_configuration_mirroring()
{
    return mock_type(bool);
}

uint8_t __wrap_system_info_get_board_rework_id()
{
    return mock_type(uint8_t);
}

uint8_t __wrap_system_info_get_soc_position()
{
    return mock_type(uint8_t);
}

void __wrap_hm_submit_cper(uint16_t error_domain_idx,
                           acpi_error_severity_t err_severity,
                           acpi_cper_section_t* err_record_section,
                           uint32_t err_record_section_size)
{
    FPFW_UNUSED(error_domain_idx);
    FPFW_UNUSED(err_severity);
    FPFW_UNUSED(err_record_section);
    FPFW_UNUSED(err_record_section_size);
    function_called();
}

uintptr_t __wrap_get_rpss_resolved_base(RPSS_INSTANCE rpss_id)
{
    assert_in_range(rpss_id, RPSS0, RPSS7);

    return mock_type(uintptr_t);
}
