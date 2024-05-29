/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file mock_pvt.c
 * Provides the mocked version of tile/soc pvt APIs necessary for power testing
 */

/*------------- Includes -----------------*/
#include "mock_pvt.h"
// clang-format off
#include <setjmp.h>
// clang-format on
#include <cmocka.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/
#define UNUSED(a) ((void)a)

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/
static tile_pvt_telem_setting_config_t s_tile_pvt_telem_settings;
static pvt_setting_config_t s_tile_pvt_settings;
static pvt_setting_config_t s_soc_pvt_settings;

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/
int __wrap_tile_pvt_init(uint32_t tile_num,
                         uintptr_t cluster_pex_base_addr,
                         const tile_pvt_telem_setting_config_t* pvt_telem_settings,
                         const pvt_setting_config_t* pvt_settings)
{
    UNUSED(tile_num);
    UNUSED(cluster_pex_base_addr);
    memcpy_s(&s_tile_pvt_settings, sizeof(s_tile_pvt_settings), pvt_settings, sizeof(pvt_setting_config_t));
    memcpy_s(&s_tile_pvt_telem_settings, sizeof(s_tile_pvt_telem_settings), pvt_telem_settings, sizeof(tile_pvt_telem_setting_config_t));
    return mock_type(int);
}

void __wrap_tile_pvt_dma_config(uintptr_t cluster_pex_base_addr, const tile_pvt_telem_setting_config_t* pvt_telem_settings)
{
    check_expected(cluster_pex_base_addr);
    memcpy_s(&s_tile_pvt_telem_settings, sizeof(s_tile_pvt_telem_settings), pvt_telem_settings, sizeof(tile_pvt_telem_setting_config_t));
}

int __wrap_soc_pvt_init(uintptr_t mscp_exp_base_address, const pvt_setting_config_t* pvt_settings)
{
    UNUSED(mscp_exp_base_address);
    memcpy_s(&s_soc_pvt_settings, sizeof(s_soc_pvt_settings), pvt_settings, sizeof(pvt_setting_config_t));
    return mock_type(int);
}

void __wrap_reset_tile_pvt_dts_vm(uintptr_t cluster_pex_base_addr)
{
    check_expected(cluster_pex_base_addr);
}

int __wrap_tile_pvt_sda_reconfig(uintptr_t cluster_pex_base_addr, bool fw_sda_ip_control)
{
    check_expected(cluster_pex_base_addr);
    check_expected(fw_sda_ip_control);
    return mock_type(int);
}

const tile_pvt_telem_setting_config_t* mock_pvt_last_tile_telem_config()
{
    return &s_tile_pvt_telem_settings;
}
const pvt_setting_config_t* mock_pvt_last_tile_config()
{
    return &s_tile_pvt_settings;
}
const pvt_setting_config_t* mock_pvt_last_soc_config()
{
    return &s_soc_pvt_settings;
}
