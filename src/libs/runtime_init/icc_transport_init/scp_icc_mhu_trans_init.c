/**
 * @file scp_icc_mhu_trans_init.c
 * Instantiates baremetal, polling based, ICC MHU - for SCP
 */

/*------------- Includes -----------------*/

#include <FpFwAssert.h>
#include <fpfw_init.h>               // for fpfw_init_get_handle, FPFW_INIT...
#include <fpfw_status.h>             // for fpfw_status_t
#include <icc_mhu.h>
#include <icc_mhu_cfg.h>
#include <icc_mhu_trans_prim.h>      // for icc mhu transport APIs and definitions
#include <idhw.h>
#include <kng_icc_shared.h>
#include <kng_scmi_shared.h>
#include <kng_scp_tfa_shared.h>
#include <stdint.h>                  // for uint32_t
#include <stdio.h>

/*-------------- Macros ------------------*/
#ifndef MAILBOX_MSCP_OFFSET
    #define MAILBOX_MSCP_OFFSET (0x60000000)
#endif

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static icc_mhu_configuration_t icc_mhu_configuration[] = {
    // SCP TO AP S Send Response -(TFA reponse)
    {
        .channel_id = MHU_INTERFACE_ID(SCP_LOCAL, AP_CORE_SEC),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_SCMI_ON_ICC,
                .direction = ICC_SEND_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MHU_SCP_AP_S_SEND_BASE_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = TFA_AP_2_SCP_RECEIVE_BASE + MAILBOX_MSCP_OFFSET,
        .mailbox_size = TFA_AP_2_SCP_RECEIVE_SIZE,
    },
    // AP TO SCP S Receive Message Configuration
    {
        .channel_id = MHU_INTERFACE_ID(AP_CORE_SEC, SCP_LOCAL),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_SCMI_ON_ICC,
                .direction = ICC_RECEIVE_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MHU_SCP_AP_S_REC_BASE_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (TFA_AP_2_SCP_SEND_BASE + MAILBOX_MSCP_OFFSET),
        .mailbox_size = SCP_2_TFA_AP_RECEIVE_SIZE,
    },
};

static icc_mhu_configuration_t die1_icc_mhu_configuration[] = {
    // SCP TO AP S Send Response -(TFA reponse)
    {
        .channel_id = MHU_INTERFACE_ID(SCP_LOCAL, AP_CORE_SEC),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_SCMI_ON_ICC,
                .direction = ICC_SEND_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MHU_SCP_AP_S_SEND_BASE_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = ARSM_GET_REGION_OFFSET(DIE1_TFA_AP_2_SCP_RECEIVE_BASE) + MAILBOX_MSCP_OFFSET,
        .mailbox_size = DIE1_TFA_AP_2_SCP_RECEIVE_SIZE,
    },
    // AP TO SCP S Receive Message Configuration
    {
        .channel_id = MHU_INTERFACE_ID(AP_CORE_SEC, SCP_LOCAL),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_SCMI_ON_ICC,
                .direction = ICC_RECEIVE_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MHU_SCP_AP_S_REC_BASE_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = ARSM_GET_REGION_OFFSET(DIE1_TFA_AP_2_SCP_SEND_BASE) + MAILBOX_MSCP_OFFSET,
        .mailbox_size = DIE1_SCP_2_TFA_AP_RECEIVE_SIZE,
    },
};

static icc_mhu_configuration_t* get_config_table(KNG_DIE_ID die)
{
    if (die == DIE_0)
    {
        return icc_mhu_configuration;
    }

    return die1_icc_mhu_configuration;
}

static uint32_t get_num_configs(KNG_DIE_ID die)
{
    if (die == DIE_0)
    {
        return sizeof(icc_mhu_configuration) / sizeof(icc_mhu_configuration_t);
    }

    return sizeof(die1_icc_mhu_configuration) / sizeof(icc_mhu_configuration_t);
}

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(icc_mhu_trans, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver"))
{
    // Initialize just the primitives for now
    KNG_DIE_ID die_num = idhw_get_die_id();
    icc_mhu_trans_init(get_config_table(die_num), get_num_configs(die_num));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

