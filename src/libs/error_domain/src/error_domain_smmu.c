//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_smmu.c
 * This file contains smmu ras related functionality.
 */

/*------------- Includes -----------------*/
#include <cper.h>
#include <error_domain_i.h>
#include <health_monitor.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <ap_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/

/**
 *  Initialize AP error domain

 */
void register_smmu_error_domain(void)
{
    // Register the error domain with the Health Monitor module
    hm_register_error_domain(ACPI_ERROR_DOMAIN_SMMU, NULL, NULL, hm_smmu_error_injection_handler, NULL);
}
