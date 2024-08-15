//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mesh.c
 *  This modules initializes various Mesh components
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <bug_check.h>
#include <cmn800_sequence.h> // for cmn800_sequence, d2dss_sequence, cmn8...
#include <cmn_config.h>      // for CMN800_CONFIG_CONFIG
#include <fpfw_icc_base.h>   // for fpfw_icc_base_ctx_t
#include <hsp_firmware_headers.h>
#include <idhw.h>              // for idhw_is_single_die_boot_en
#include <idsw.h>              // for idsw_get_platform_sdv,
#include <idsw_kng.h>          // for PLATFORM_FPGA_LARGE
#include <kng_soc_constants.h> // for NUM_DIE
#include <mesh.h>              // for mesh_init
#include <stdbool.h>           // for true
#include <stdint.h>            // for uint8_t
#include <stdio.h>             // for printf
#include <system_info.h>

/*------------- Defines -----------------*/
bool dual_die_boot = false; // ADO: 1825901 Deprecate after ADO is implemented by SVP
static fpfw_icc_base_ctx_t* s_mbx_icc_ctx;

/*------------- Functions ----------------*/
static void hsp_send_recv_enable_smmu()
{
    size_t recv_msg_size_bytes = 0;
    kng_hsp_mailbox_msg msg = {
        .header.cmd = HSP_MAILBOX_CMD_ENABLE_SMMU_ACCESS_REQ,
    };

    //! Send the message to HSP & get response, blocking call
    fpfw_status_t icc_status =
        fpfw_icc_base_send_recv_sync(s_mbx_icc_ctx, &msg, sizeof(kng_hsp_mailbox_msg), &recv_msg_size_bytes);

    //! Verify sync return status & response message
    BUG_ASSERT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS);
    BUG_ASSERT(recv_msg_size_bytes > 0);
    BUG_ASSERT(msg.header.cmd == HSP_MAILBOX_CMD_ENABLE_SMMU_ACCESS_RSP);
    BUG_ASSERT(msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS);
}

void mesh_init(uint8_t die_num, fpfw_icc_base_ctx_t* icc_ctx)
{
    FPFW_RUNTIME_ASSERT(die_num < NUM_DIE);

    s_mbx_icc_ctx = icc_ctx;

    int sts = 0x0;

    cmn800_sequence_params_t cmn800_sequence_param = {};
    cmn800_sequence_param.die_num = die_num;
    cmn800_sequence_param.cmn_config_enum = CONFIG_1D_UMA_64HNS_HIER_3SN_enum;
    cmn800_sequence_param.CMN_INIT = 2; // CMN_INIT_FRONTDOOR_HNS_PPU_POLL_CMN_SAM_CFG
    cmn800_sequence_param.DRAM_SIZE = DRAM_32GB;
    cmn800_sequence_param.BOOT_2D_ENABLE = 0;
    cmn800_sequence_param.HNS_SPARE_DIE0 = 0; // CMN_HNS_SPARE_CONFIG0_X_X_X_X
    cmn800_sequence_param.HNS_SPARE_DIE1 = 0; // CMN_HNS_SPARE_CONFIG0_X_X_X_X
    cmn800_sequence_param.D2D_INIT = 0;       // D2D_INIT_FRONTDOOR
    cmn800_sequence_param.USE_HW_TIMER = 0;

    if (dual_die_boot == true) // ADO: 1825901 Deprecate after ADO is implemented by SVP
    {
        if (!idhw_is_single_die_boot_en()) // 2 Die
        {
            printf("Dual Die Boot\n");
            cmn800_sequence_param.cmn_config_enum = CONFIG_2D_NUMA_64HNS_HIER_3SN_enum; // 2 Die
            cmn800_sequence_param.BOOT_2D_ENABLE = true;
        }
    }
    if (idsw_get_platform_sdv() == PLATFORM_FPGA_LARGE)
    {
        if (idhw_is_single_die_boot_en())
        {
            cmn800_sequence_param.cmn_config_enum = CONFIG_1D_UMA_8HNS_HIER_3SN_enum; // 1 Die FPGA
        }
        else
        {
            cmn800_sequence_param.cmn_config_enum = CONFIG_2D_NUMA_8HNS_HIER_3SN_enum; // 2 Die FPGA
            cmn800_sequence_param.BOOT_2D_ENABLE = true;
        }
    }
    printf("cmn800_sequence_param.cmn_config_enum 0x%x\n", (uint8_t)cmn800_sequence_param.cmn_config_enum);

    sts = cmn800_sequence_svp_updates(cmn800_sequence_param);
    printf("cmn800_sequence_svp_updates sts 0x%x\n", sts);
    FPFW_RUNTIME_ASSERT(sts == 0);

    sts = cmn800_sequence(cmn800_sequence_param);
    printf("cmn800_sequence sts 0x%x\n", sts);
    FPFW_RUNTIME_ASSERT(sts == 0);

    if (system_info_is_hsp_present())
    {
        printf("Send enable SMMU in bypass mode\n");
        hsp_send_recv_enable_smmu();
    }

    if (cmn800_sequence_param.BOOT_2D_ENABLE)
    {
        sts = d2dss_sequence(cmn800_sequence_param);
        printf("d2dss_sequence sts 0x%x\n", sts);
        FPFW_RUNTIME_ASSERT(sts == 0);
    }
    else
    {
        printf("Skip d2dss_sequence\n");
    }

    // ADO 1728673
    // Send HSP Mailbox message to confirm SCP is done with Mesh and D2D Init
    // {
    //     hw_debug("// sending scp done to HSP\n");
    //     HSP_MAILBOX_MSG mbx_msg = {.Cmd = HspMailboxCmdCmlReady};
    //     while (!sp_mbx_send(SCP_HSP_MBX_ADDRESS, (uint32_t *)&mbx_msg, 0x4))
    //         ;
    //     // hold the SCP until the HSP is done
    //     while (!sp_mbx_receive(SCP_HSP_MBX_ADDRESS, (uint32_t *)&mbx_msg, 0x4))
    //         ;
    //     hw_assert(mbx_msg.Cmd == HspMailboxCmdCmlReady);
    //     hw_assert(SP_SUCCEEDED(mbx_msg.ReturnStatus.Status));
    //     hw_debug("// HSP acks\n");
    // }
}
