//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file css.c
 *  This modules initializes various CSS components
 */

/*------------- Includes -----------------*/

#include <FpFwAssert.h>
#include <clocks_sequence.h>
#include <css.h>
#include <ppu_v1.h>
#include <stdbool.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// TODO: WI 1726113 Create a component initialization library for these APIs
void css_pre_mesh_init()
{
    clocks_sequence_css_pre_mesh_init_t scp_clocks_pre_mesh_param = {};
    scp_clocks_pre_mesh_param.system_ppu_opmode = PPU_V1_OPMODE_01;
    scp_clocks_pre_mesh_param.skip_css_pcr_init = false;
    scp_clocks_pre_mesh_param.skip_msxp_pcr_init = false;
    int sts = clocks_sequence_css_pre_mesh_init(&scp_clocks_pre_mesh_param);
    FPFW_RUNTIME_ASSERT(sts == 0);
}

void css_post_mesh_init()
{
    clocks_sequence_css_post_mesh_init_t scp_clocks_post_mesh_param = {};
    scp_clocks_post_mesh_param.skip_periph_pcr_init = false;
    int sts = clocks_sequence_css_post_mesh_init(&scp_clocks_post_mesh_param);
    FPFW_RUNTIME_ASSERT(sts == 0);
}