//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_error_handling.c
 *
 * Implements PCIe error and RAS event handling functions as well as invokes
 * the relevant APIs to log CPER and/or SEL events.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <common_types.h>
#include <cper.h>
#include <pcie_sync_requests_i.h>
#include <ras_common.h>
#include <scp_pcie_manager.h>
#include <silibs_status.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static const guid_t pcie_vendor_defined_error_domain_guid = ACPI_ERROR_TYPE_VENDOR_DEFINED_PCIE;

/*------------- Functions ----------------*/
const guid_t* get_pcie_vendor_defined_error_domain_guid(void)
{
    return &pcie_vendor_defined_error_domain_guid;
}

void handle_pcie_vsecras_event(pcie_manager_context_t* ctx, pciess_completion_request_t* cmpl)
{
    RPSS_INSTANCE rpss_idx = ctx->rpss_idx;
    uint8_t rp_idx = cmpl->rp_index;
    ras_error_record_t error_record = {0};

    while (send_sync_rp_probe_vsecras((PDFWK_INTERFACE_HEADER)(ctx->iface), rpss_idx, rp_idx, &error_record) == SILIBS_E_DEVICE)
    {
        ras_print_record(&error_record);
        memset(&error_record, 0, sizeof(ras_error_record_t));
    }
}

void handle_pcie_dtim_event(pcie_manager_context_t* ctx, pciess_completion_request_t* cmpl)
{
    RPSS_INSTANCE rpss_idx = ctx->rpss_idx;
    uint8_t rp_idx = cmpl->rp_index;
    ras_error_record_t error_record = {0};

    while (send_sync_rp_probe_dtim((PDFWK_INTERFACE_HEADER)(ctx->iface), rpss_idx, rp_idx, &error_record) == SILIBS_E_DEVICE)
    {
        ras_print_record(&error_record);
        memset(&error_record, 0, sizeof(ras_error_record_t));
    }
}

void handle_pcie_ltim_event(pcie_manager_context_t* ctx, pciess_completion_request_t* cmpl)
{
    RPSS_INSTANCE rpss_idx = ctx->rpss_idx;
    uint8_t rp_idx = cmpl->rp_index;
    ras_error_record_t error_record = {0};

    while (send_sync_rp_probe_ltim((PDFWK_INTERFACE_HEADER)(ctx->iface), rpss_idx, rp_idx, &error_record) == SILIBS_E_DEVICE)
    {
        ras_print_record(&error_record);
        memset(&error_record, 0, sizeof(ras_error_record_t));
    }
}
