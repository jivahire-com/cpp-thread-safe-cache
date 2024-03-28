//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mesh.c
 *  This modules initializes various Mesh components
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>
#include <cmn800_sequence.h>
#include <cmn_config.h>
#include <kng_soc_constants.h>
#include <mesh.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Functions ----------------*/

void mesh_init(uint8_t die_num)
{
    FPFW_RUNTIME_ASSERT(die_num < NUM_DIE);

    int sts = 0x0;

    cmn800_sequence_params_t cmn800_sequence_param = {};
    cmn800_sequence_param.die_num = die_num;
    cmn800_sequence_param.cmn_config_enum = CONFIG_1D_UMA_64HNS_HIER_3SN_enum;
    cmn800_sequence_param.CMN_INIT = 2;
    cmn800_sequence_param.BOOT_2D_ENABLE = 0;
    cmn800_sequence_param.HNS_SPARE_DIE0 = 0;
    cmn800_sequence_param.HNS_SPARE_DIE1 = 0;
    cmn800_sequence_param.D2D_INIT = 0;
    cmn800_sequence_param.SKIP_PERIPH_TOWER_INIT = 0;
    cmn800_sequence_param.SKIP_FABRIC_TOWER_INIT = 0;

    sts = cmn800_sequence(cmn800_sequence_param);
    printf("cmn800_sequence sts 0x%x\n", sts);
    FPFW_RUNTIME_ASSERT(sts == 0);

    // ADO 1728673
    // Wait for HSP to configure system SMMU in Bypass mode (TBU feeding into RND-5)
    // so that SCP addresses (e.g. D2D related CSR read/writes) make it thru the mesh to the HNT
    // HSP_MAILBOX_MSG mbx_msg = {};
    // mbx_msg.Cmd = HspMailboxCmdSMMUEnableAccess;
    // while (!sp_mbx_send(SCP_HSP_MBX_ADDRESS, (uint32_t *)&mbx_msg, 0x4))
    //     ;
    // while (!sp_mbx_receive(SCP_HSP_MBX_ADDRESS, (uint32_t *)&mbx_msg, 0x4))
    //     ;
    // hw_assert(SP_SUCCEEDED(mbx_msg.ReturnStatus.Status));

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
