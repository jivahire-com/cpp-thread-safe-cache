/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file hm_test.h
 *
 */

#pragma once

/*----------- Includes ------------*/
#include <CMockaWrapper.h>
#include <health_monitor.h>
#include <health_monitor_i.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
int pre_ddr_setup(void** state);
int post_ddr_setup(void** state);
acpi_einj_cmd_status_t hm_error_injection_cb(ras_einj_info_t* payload, void* ctx);
