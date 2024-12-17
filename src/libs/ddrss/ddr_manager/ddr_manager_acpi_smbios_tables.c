//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_acpi_smbios_tables.c
 *
 * This file will create BDAT and SMBIOS Type 16/17 tables for UEFI
 */

/*------------- Includes -----------------*/
#include "ddr_manager_i.h"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void ddr_create_bdat(void)
{
    // Get DDR training data from ddrss silicon libs
    // Create ACPI BDAT
    // Copy to reserved DDR region specified by shared header file
}

void ddr_create_smbios_tables(void)
{
    // Get each DIMM's SPD data  from ddrss silicon libs - or read via I3C
    // Get relevant config data (number of DIMMs, rank, speed, etc.)
    // Create SMBIOS Type 16/17
    // Copy to reserved DDR region specified by shared header file

    ddr_create_smbios_type_16();
    copy_type_16_to_ddr();

    ddr_create_smbios_type_17();
    copy_type_17_to_ddr();
}

void ddr_create_smbios_type_16(void)
{
    // Create SMBIOS Type 16
}

void ddr_create_smbios_type_17(void)
{
    // Create SMBIOS Type 17
}

void copy_type_16_to_ddr(void)
{
    // Copy SMBIOS Type 16 to DDR
}

void copy_type_17_to_ddr(void)
{
    // Copy SMBIOS Type 17 to DDR
}