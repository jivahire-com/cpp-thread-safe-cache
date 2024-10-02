/**
 * @file scp_icc_mhu_trans_init.c
 * Instantiates baremetal, polling based, ICC MHU - for SCP
 */

/*------------- Includes -----------------*/

#include <DfwkClient.h>
#include <DfwkHost.h>                // for DfwkDeviceInitialize
#include <DfwkThreadXHost.h>         // for PDFWK_THREADX_HOST
#include <FpFwAssert.h>
#include <atu_api.h>
#include <fpfw_init.h>               // for fpfw_init_get_handle, FPFW_INIT...
#include <fpfw_status.h>             // for fpfw_status_t
#include <icc_mhu.h>
#include <icc_mhu_cfg.h>
#include <icc_mhu_trans_dfwk.h>      // for icc mhu transport APIs and definitions
#include <icc_mhu_trans_prim.h>      // for icc mhu transport APIs and definitions
#include <idhw.h>
#include <kng_icc_shared.h>
#include <kng_scmi_shared.h>
#include <kng_scp_tfa_shared.h>
#include <stdint.h>                  // for uint32_t
#include <stdio.h>

/*-------------- Macros ------------------*/

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
        .mailbox_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(D0_ARSM_TFA_AP_2_SCP_RECEIVE_BASE),
        .mailbox_size = D0_ARSM_TFA_AP_2_SCP_RECEIVE_SIZE,
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
        .mailbox_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(D0_ARSM_TFA_AP_2_SCP_SEND_BASE),
        .mailbox_size = D0_ARSM_TFA_AP_2_SCP_SEND_SIZE,
    },
    // SCP TO MCP Send Message/ Send response Configuration
    {
        .channel_id = MHU_INTERFACE_ID(SCP_LOCAL, MCP_LOCAL),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_STANDARD,
                .direction = ICC_SEND_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MHU_SCP_MCP_SEND_BASE_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (SCP_2_MCP_SEND_BASE_SMT),
        .mailbox_size = SCP_2_MCP_SMT_SIZE,
    },
    // MCP TO SCP (SCP Receive) Message Configuration
    {
        .channel_id = MHU_INTERFACE_ID(MCP_LOCAL,SCP_LOCAL),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_STANDARD,
                .direction = ICC_RECEIVE_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MHU_SCP_MCP_REC_BASE_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (SCP_2_MCP_RECEIVE_BASE_SMT),
        .mailbox_size = SCP_2_MCP_SMT_SIZE,
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
        .mailbox_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D1_ARSM_TFA_AP_2_SCP_RECEIVE_BASE),
        .mailbox_size = D1_ARSM_TFA_AP_2_SCP_RECEIVE_SIZE,
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
        .mailbox_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D1_ARSM_TFA_AP_2_SCP_SEND_BASE),
        .mailbox_size = D1_ARSM_TFA_AP_2_SCP_SEND_SIZE,
    },
    // SCP TO MCP Send Message/ Send response Configuration
    {
        .channel_id = MHU_INTERFACE_ID(SCP_LOCAL, MCP_LOCAL),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_STANDARD,
                .direction = ICC_SEND_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MHU_SCP_MCP_SEND_BASE_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (SCP_2_MCP_SEND_BASE_SMT),
        .mailbox_size = SCP_2_MCP_SMT_SIZE,
    },
    // MCP TO SCP (SCP Receive) Message Configuration
    {
        .channel_id = MHU_INTERFACE_ID(MCP_LOCAL,SCP_LOCAL),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_STANDARD,
                .direction = ICC_RECEIVE_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MHU_SCP_MCP_REC_BASE_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (SCP_2_MCP_RECEIVE_BASE_SMT),
        .mailbox_size = SCP_2_MCP_SMT_SIZE,
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
FPFW_INIT_COMPONENT(icc_mhu_trans, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "atu_svc"))
{
    // Initialize just the primitives for now
    KNG_DIE_ID die_num = idhw_get_die_id();
    icc_mhu_trans_init(get_config_table(die_num), get_num_configs(die_num));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(icc_scp_tfa, FPFW_INIT_DEPENDENCIES("dfwk"))
{
    fpfw_init_component_id_t dfwk_id = "dfwk";
    static icc_mhu_transport_device_t icc_mhu_dev = {};
    static icc_mhu_transport_config_t config = {
        .send_core_2_core_id = MHU_INTERFACE_ID(SCP_LOCAL, AP_CORE_SEC),
        .receive_core_2_core_id = MHU_INTERFACE_ID(AP_CORE_SEC, SCP_LOCAL),
    };

    // Initialize the driver framework device
    fpfw_status_t status = icc_mhu_transport_dfwk_device_init(&icc_mhu_dev, &((PDFWK_THREADX_HOST)fpfw_init_get_handle(dfwk_id))->Schedule, &config);
    return (fpfw_init_result_t){status, &icc_mhu_dev};
}

FPFW_INIT_COMPONENT(icc_scp_tfa_intf, FPFW_INIT_DEPENDENCIES("icc_scp_tfa"))
{
    static icc_mhu_transport_intrf_t icc_mhu_intf_tfa_2_scp = {
    };
    fpfw_init_component_id_t icc_mhu_transport_id = "icc_scp_tfa";

    // get mhu transport's device handle and initialize the interface
    icc_mhu_transport_device_t* icc_mhu_handle = (icc_mhu_transport_device_t*)fpfw_init_get_handle(icc_mhu_transport_id);
    fpfw_status_t status = icc_mhu_trans_dfwk_interface_init(icc_mhu_handle, &icc_mhu_intf_tfa_2_scp);
    DfwkClientInterfaceOpen(&icc_mhu_intf_tfa_2_scp.header);
    return (fpfw_init_result_t){status, &icc_mhu_intf_tfa_2_scp};
}

FPFW_INIT_COMPONENT(icc_scp_mcp, FPFW_INIT_DEPENDENCIES("dfwk"))
{
    fpfw_init_component_id_t dfwk_id = "dfwk";
    static icc_mhu_transport_device_t icc_mhu_dev2 = {};
    static icc_mhu_transport_config_t config = {
        .send_core_2_core_id = MHU_INTERFACE_ID(SCP_LOCAL, MCP_LOCAL),
        .receive_core_2_core_id = MHU_INTERFACE_ID(MCP_LOCAL, SCP_LOCAL),
    };


    // Initialize the driver framework device
    fpfw_status_t status = icc_mhu_transport_dfwk_device_init(&icc_mhu_dev2, &((PDFWK_THREADX_HOST)fpfw_init_get_handle(dfwk_id))->Schedule, &config);
    return (fpfw_init_result_t){status, &icc_mhu_dev2};
}

FPFW_INIT_COMPONENT(icc_scp_mcp_intf, FPFW_INIT_DEPENDENCIES("icc_scp_mcp"))
{
    static icc_mhu_transport_intrf_t icc_mhu_intf_scp_2_mcp = {
    };
    fpfw_init_component_id_t icc_mhu_transport_id_2 = "icc_scp_mcp";

    // get mhu transport's device handle and initialize the interface
    icc_mhu_transport_device_t* icc_mhu_handle = (icc_mhu_transport_device_t*)fpfw_init_get_handle(icc_mhu_transport_id_2);
    fpfw_status_t status = icc_mhu_trans_dfwk_interface_init(icc_mhu_handle, &icc_mhu_intf_scp_2_mcp);
    DfwkClientInterfaceOpen(&icc_mhu_intf_scp_2_mcp.header);
    return (fpfw_init_result_t){status, &icc_mhu_intf_scp_2_mcp};
}