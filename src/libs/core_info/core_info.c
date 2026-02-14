//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file core_info.c
 * Implements system info APIs
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <assert.h>
#include <bug_check.h>
#include <core_info.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <fpfw_status.h>   // for fpfw_status_t
#include <fuse.h>          // fuse library functions
#include <fuse_init.h>
#include <fuse_struct.h>
#include <fuses_top_regs.h>
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#include <idsw.h>                 // for idsw_get_platform_sdv
#include <idsw_kng.h>             // for idsw_get_platform_sdv
#include <inttypes.h>
#include <kingsgate_hsp_mailbox_commands.h>
#define __NO_CSR_TYPEDEFS__
#include <scp_exp_top_regs.h>
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <shared_crashdump_def.h>
#include <shared_sds_def.h> //Fuse SDS block and struct id
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static corebits_t sys_cores_in_die;

static etc_core_identifier_t core_id = {.die_id = 0xFFFF, .core_id = 0xFFFF}; // Initialize to invalid value

/*------------- Functions ----------------*/
void core_info_get_platform_disable_cores()
{

    printf("Start process disable cores.\n");
    kng_fuse_disable_core_t p_fuse_disable = {};
    kng_fuse_disable_core_t p_config_disable = {};
    kng_fuse_disable_core_t p_config_spare_en = {};
    kng_fuse_disable_core_t p_result_disable = {};
    kng_fuse_disable_core_t p_result_enable = {};
    if (idsw_get_die_id() == DIE_0)
    {
        p_config_disable.fuse_dis_core_31_0 = config_get_die0_core_disable_value_31_0();
        p_config_disable.fuse_dis_core_63_32 = config_get_die0_core_disable_value_63_32();
        p_config_disable.fuse_dis_core_95_64 = config_get_die0_core_disable_value_95_64();

        p_config_spare_en.fuse_dis_core_31_0 = config_get_die0_core_spare_en_31_0();
        p_config_spare_en.fuse_dis_core_63_32 = config_get_die0_core_spare_en_63_32();
        p_config_spare_en.fuse_dis_core_95_64 = config_get_die0_core_spare_en_95_64();
    }
    else
    {
        p_config_disable.fuse_dis_core_31_0 = config_get_die1_core_disable_value_31_0();
        p_config_disable.fuse_dis_core_63_32 = config_get_die1_core_disable_value_63_32();
        p_config_disable.fuse_dis_core_95_64 = config_get_die1_core_disable_value_95_64();

        p_config_spare_en.fuse_dis_core_31_0 = config_get_die1_core_spare_en_31_0();
        p_config_spare_en.fuse_dis_core_63_32 = config_get_die1_core_spare_en_63_32();
        p_config_spare_en.fuse_dis_core_95_64 = config_get_die1_core_spare_en_95_64();
    }

    read_core_defect_fuses(&(p_fuse_disable.fuse_dis_core_95_64),
                           &(p_fuse_disable.fuse_dis_core_63_32),
                           &(p_fuse_disable.fuse_dis_core_31_0));
    p_result_disable.fuse_dis_core_31_0 = ~((p_fuse_disable.fuse_dis_core_31_0 | p_config_disable.fuse_dis_core_31_0));
    p_result_disable.fuse_dis_core_63_32 =
        ~((p_fuse_disable.fuse_dis_core_63_32 | p_config_disable.fuse_dis_core_63_32));
    p_result_disable.fuse_dis_core_95_64 =
        ~((p_fuse_disable.fuse_dis_core_95_64 | p_config_disable.fuse_dis_core_95_64)) & 0x0F;
    p_result_enable.fuse_dis_core_31_0 = ~(p_result_disable.fuse_dis_core_31_0);
    p_result_enable.fuse_dis_core_63_32 = ~(p_result_disable.fuse_dis_core_63_32);
    p_result_enable.fuse_dis_core_95_64 = ~(p_result_disable.fuse_dis_core_95_64);
    fuse_disable_cores_to_66(&p_result_enable); // this function assumes 1 is disabled and 0 is enabled, so we need to invert the bits
    p_result_disable.fuse_dis_core_31_0 = ~(p_result_enable.fuse_dis_core_31_0);
    p_result_disable.fuse_dis_core_63_32 = ~(p_result_enable.fuse_dis_core_63_32);
    p_result_disable.fuse_dis_core_95_64 = ~(p_result_enable.fuse_dis_core_95_64);
    printf("Before Spare Core DIE [%d] : core0-31=0x%" PRIx32 " core32-63=0x%" PRIx32 " core64-95=0x%" PRIx32 " \n",
           idsw_get_die_id(),
           (p_result_disable.fuse_dis_core_31_0),
           (p_result_disable.fuse_dis_core_63_32),
           (p_result_disable.fuse_dis_core_95_64));
    // Task 2949783 Apply spare core knob AFTER 1 or 2 core disable logic
    p_result_disable.fuse_dis_core_31_0 |= p_config_spare_en.fuse_dis_core_31_0;
    p_result_disable.fuse_dis_core_63_32 |= p_config_spare_en.fuse_dis_core_63_32;
    p_result_disable.fuse_dis_core_95_64 =
        ((p_result_disable.fuse_dis_core_95_64 | p_config_spare_en.fuse_dis_core_95_64) & 0x0F);
    // end

    printf("DIE [%d] : core0-31=0x%" PRIx32 " core32-63=0x%" PRIx32 " core64-95=0x%" PRIx32 " \n",
           idsw_get_die_id(),
           p_result_disable.fuse_dis_core_31_0,
           p_result_disable.fuse_dis_core_63_32,
           p_result_disable.fuse_dis_core_95_64);
    sys_cores_in_die = (corebits_t)COREBITS_INIT_UINT32(p_result_disable.fuse_dis_core_31_0,
                                                        p_result_disable.fuse_dis_core_63_32,
                                                        p_result_disable.fuse_dis_core_95_64);
}

corebits_t* core_info_get_enable_cores_result()
{
    return (&sys_cores_in_die);
}

void core_info_init()
{
    core_info_get_platform_disable_cores();

    /* Initialize core info */
    core_id.die_id = idsw_get_die_id();
    core_id.core_id = idsw_get_cpu_type();

    /* Translate to the ID in crash_dump_core_t */
    switch (core_id.core_id)
    {
    case CPU_MCP:
        core_id.core_id = CRASH_DUMP_CORE_MCP;
        break;
    case CPU_SCP:
        core_id.core_id = CRASH_DUMP_CORE_SCP;
        break;
    case CPU_SDM:
        core_id.core_id = CRASH_DUMP_CORE_SDM;
        break;
    case CPU_CDED_SDM:
        core_id.core_id = CRASH_DUMP_CORE_CDED;
        break;
    default:
        BUG_ASSERT(false);
    }

    FPFW_DBGPRINT_INFO("Core info init, Die[%d] : Core[0x%" PRIx32 "]\n", core_id.die_id, core_id.core_id);
}

uint32_t core_info_get_combined_core_id()
{
    return core_id.combined_id;
}