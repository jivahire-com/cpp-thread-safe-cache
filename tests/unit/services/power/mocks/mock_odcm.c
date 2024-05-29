/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file mock_odcm.c
 * Provides the mocked version odcm APIs necessary for power testing
 */

/*------------- Includes -----------------*/
#include "mock_odcm.h"
// clang-format off
#include <setjmp.h>
// clang-format on
#include <cmocka.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/
#define UNUSED(a) ((void)a)

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/
static odcm_config_t last_odcm_config = {0};

/*-------- Function Prototypes -----------*/
/*------------- Functions ----------------*/
int __wrap_odcm_init(const uintptr_t cluster_pex_base_addr, const odcm_config_t* odcm_cfg)
{
    UNUSED(cluster_pex_base_addr);
    memcpy_s(&last_odcm_config, sizeof(last_odcm_config), odcm_cfg, sizeof(odcm_config_t));
    return mock_type(int);
}

void __wrap_odcm_telemetry_config(const uintptr_t cluster_pex_base_addr, const odcm_telem_config_t* telem_cfg)
{
    check_expected(cluster_pex_base_addr);
    memcpy_s(&last_odcm_config.telem_cfg, sizeof(last_odcm_config.telem_cfg), telem_cfg, sizeof(odcm_telem_config_t));
}

const odcm_config_t* mock_odcm_last_config()
{
    return &last_odcm_config;
}
