//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file system_info.c
 * Implements system info APIs
 */

/*------------- Includes -----------------*/
#include <assert.h>
#include <core_info.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <fpfw_status.h>   // for fpfw_status_t
#include <fuse.h>          // fuse library functions
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
#include <shared_sds_def.h> //Fuse SDS block and struct id
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static corebits_t sys_cores_in_die;

/*------------- Functions ----------------*/
void core_info_get_platform_disable_cores()
{

    printf("Start process disable cores.\n");
    kng_fuse_disable_core_t p_fuse_disable = {};
    kng_fuse_disable_core_t p_config_disable = {};
    kng_fuse_disable_core_t p_config_spare_en = {};
    kng_fuse_disable_core_t p_result_disable = {};
    p_config_disable.fuse_dis_core_0_31 = config_get_core_disable_value_0_31();
    p_config_disable.fuse_dis_core_32_63 = config_get_core_disable_value_32_63();
    p_config_disable.fuse_dis_core_64_95 = config_get_core_disable_value_64_95();

    p_config_spare_en.fuse_dis_core_0_31 = config_get_core_spare_en_0_31();
    p_config_spare_en.fuse_dis_core_32_63 = config_get_core_spare_en_32_63();
    p_config_spare_en.fuse_dis_core_64_95 = config_get_core_spare_en_64_95();

    read_core_defect_fuses(&(p_fuse_disable.fuse_dis_core_64_95),
                           &(p_fuse_disable.fuse_dis_core_32_63),
                           &(p_fuse_disable.fuse_dis_core_0_31));
    p_result_disable.fuse_dis_core_0_31 = ~((p_fuse_disable.fuse_dis_core_0_31 | p_config_disable.fuse_dis_core_0_31) &
                                            (~p_config_spare_en.fuse_dis_core_0_31));
    p_result_disable.fuse_dis_core_32_63 = ~((p_fuse_disable.fuse_dis_core_32_63 | p_config_disable.fuse_dis_core_32_63) &
                                             (~p_config_spare_en.fuse_dis_core_32_63));
    p_result_disable.fuse_dis_core_64_95 =
        ~(((p_fuse_disable.fuse_dis_core_64_95 | p_config_disable.fuse_dis_core_64_95) & (~p_config_spare_en.fuse_dis_core_64_95)) &
          0x0F);

    printf("core0-31=0x%" PRIx32 " core32-63=0x%" PRIx32 " core64-95=0x%" PRIx32 " \n",
           p_result_disable.fuse_dis_core_0_31,
           p_result_disable.fuse_dis_core_32_63,
           p_result_disable.fuse_dis_core_64_95);
    sys_cores_in_die = (corebits_t)COREBITS_INIT_UINT32(p_result_disable.fuse_dis_core_0_31,
                                                        p_result_disable.fuse_dis_core_32_63,
                                                        p_result_disable.fuse_dis_core_64_95);
}

corebits_t* core_info_get_enable_cores_result()
{
    return (&sys_cores_in_die);
}

void core_info_init()
{
    core_info_get_platform_disable_cores();
    printf("Core info init\n");
}