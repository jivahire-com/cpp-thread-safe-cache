//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_gic.c
 * This file contains gic ras related functionality.
 */

/*------------- Includes -----------------*/
#include <cper.h>
#include <error_domain_i.h>
#include <health_monitor.h>

/*-- Symbolic Constant Macros (defines) --*/
#define GIC_FRU "GIC"
#define ACPI_ERROR_TYPE_GIC                                \
    {                                                      \
        0x3f9d6b20, 0x482e, 0x4c3f,                        \
        {                                                  \
            0xa1, 0x72, 0x8b, 0x5c, 0xf4, 0x92, 0x6e, 0xd8 \
        }                                                  \
    }

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static guid_t GIC_ERROR_DOMAIN_GUID = ACPI_ERROR_TYPE_GIC;

/*-------------- Functions ---------------*/
void register_gic_error_domain()
{
    // Register the error domain
    hm_register_error_domain(ACPI_ERROR_DOMAIN_GIC, &GIC_ERROR_DOMAIN_GUID, GIC_FRU, gic_error_injection_handler, NULL);
}
