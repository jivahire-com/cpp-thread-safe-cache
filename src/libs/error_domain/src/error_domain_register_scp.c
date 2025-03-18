//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_register_scp.c
 * This file contains scp ras related functionality.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <health_monitor.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SCP_PROC_FRU "SCP_PROC"
#define SCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF                 \
    {                                                      \
        0xedf48164, 0x1d29, 0x47dc,                        \
        {                                                  \
            0xa7, 0x25, 0x8d, 0x8b, 0x79, 0xbb, 0xfc, 0x78 \
        }                                                  \
    }
/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static guid_t SCP_ERROR_DOMAIN_GUID = SCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF;

/*-------------- Functions ---------------*/

static acpi_einj_cmd_status_t scp_error_injection_handler(ras_einj_info_t_temp* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);
    FPFW_UNUSED(einj_payload);

    // Placeholder for SCP error injection handling
    return ACPI_EINJ_SUCCESS;
}

void register_scp_error_domain()
{
    hm_register_error_domain(ACPI_ERROR_DOMAIN_SCP_PROC, &SCP_ERROR_DOMAIN_GUID, SCP_PROC_FRU, scp_error_injection_handler, NULL);
}