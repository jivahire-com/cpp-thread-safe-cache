//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ioss_ini.c
 *  This modules initializes IOSS
 */

/*------------- Includes -----------------*/
#include "ioss_ini.h"

#include <FpFwAssert.h>
#include <atu_lib.h>
#include <fpfw_cfg_mgr.h>
#include <ioss_init.h>
#include <kng_atu_mappings.h>
#include <kng_error.h>
#include <kng_soc_constants.h>
#include <mscp_exp_rmss_memory_map.h>
#include <silibs_status.h>
#include <stdint.h>
#include <stdio.h>
#include <usb_config_variable.h>
#include <usb_knobs.h>
#include <usb_struct_defaults.h>
#include <variable_services.h>

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/
// D0-IOSS0
static atu_map_entry_t atu_ioss_map = ATU_MAPPING_IOSS_TOP(0);

static USB_CFG_DATA uefi_knobs;
static usb_cfg_t usb_prod_knobs = DEFAULT_USB_CFG_T;
static var_service_req_ctx_t req_ctx = {};
static var_service_shared_mem_t mem_ctx = {0};
static gpio_cfg_t gpio_prod_knobs = {
    .gpio_init = true,
    .gpio_afm_init = true,
};

static ioss_cfg_t ioss_init_knobs = {
    .ioss_init = true,
};

/*------------- Functions ----------------*/
void ioss_ini()
{
    silibs_status_t sts = SILIBS_SUCCESS;

    sts = atu_map(ATU_ID_MSCP, &atu_ioss_map);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    uint32_t resolved_ioss_base_addr = atu_ioss_map.mscp_start_address;

    // Override USB and GPIO knobs
    usb_prod_knobs = config_get_usb_knobs();
    gpio_prod_knobs = config_get_gpio_knobs();

    mem_ctx.payload_base = (uintptr_t)SCP_EXP_SCP_USB_VARIABLE_SERVICE_PAYLOAD_BASE;
    mem_ctx.max_payload_size = SCP_EXP_SCP_USB_VARIABLE_SERVICE_PAYLOAD_SIZE;

    // USB configuration published by SCP for UEFI
    uefi_knobs.PowerOnPort0 = usb_prod_knobs.power_on_port0;
    uefi_knobs.PowerOnPort1 = usb_prod_knobs.power_on_port1;

    uint16_t usb_var_name[] = USB_VAR_NAME;

    if (variable_service_initialize_ctx(&req_ctx, &mem_ctx) != KNG_SUCCESS)
    {
        printf("Variable_service_initialize_ctx failed");
        FPFW_RUNTIME_ASSERT(false);
    }

    // Prepare the set variable request
    var_service_req_params_t set_var_req = {
        .variable_name_ptr = usb_var_name,
        .variable_name_size = sizeof(usb_var_name),
        .vendor_namespace_guid = USB_VAR_GUID,
        .data_size = sizeof(USB_CFG_DATA),
        .data = (uint8_t*)&uefi_knobs,
        .attributes =
            {
                .as_uint32 = EFI_VARIABLE_BOOTSERVICE_ACCESS,
            },
    };

    // Publish USB variable
    variable_service_sync_set_variable(&req_ctx, &set_var_req);

    ioss_init_t init = {.ioss_knobs = &ioss_init_knobs,
                        .gpio_knobs = &gpio_prod_knobs,
                        .usb_knobs = &usb_prod_knobs,
                        .ioss_base_addr = resolved_ioss_base_addr,
                        .gpio_cfg_tbl = NULL,
                        .gpio_cfg_num = 0,
                        .gpio_afm_cfg_tbl = NULL,
                        .gpio_afm_cfg_num = 0};

    // Enable IOSS IPs
    sts = ioss_init(D0_IOSS, &init);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    sts = atu_unmap(ATU_ID_MSCP, &atu_ioss_map);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);
}