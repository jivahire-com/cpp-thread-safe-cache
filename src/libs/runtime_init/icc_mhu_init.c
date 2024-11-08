//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_mhu_init.c
 * Initializes the MHU Devices, Transports, and ICC Base contexts for the MHU Channels
 */

/*------------- Includes -----------------*/

#include <DfwkClient.h>
#include <DfwkHost.h>
#include <DfwkThreadXHost.h>
#include <FpFwAssert.h>
#include <atu_api.h>
#include <fpfw_icc_base.h>
#include <fpfw_icc_base_i.h>
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <icc_mhu.h>
#include <icc_mhu_cfg.h>
#include <icc_mhu_trans_dfwk.h>
#include <icc_mhu_trans_prim.h>
#include <idhw.h>
#include <interrupts.h>
#include <kng_icc_shared.h>
#include <kng_scmi_shared.h>
#include <kng_scp_tfa_shared.h>
#include <kng_soc_constants.h>
#include <limits.h>
#include <mscp_exp_rmss_memory_map.h>
#include <silibs_mcp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdint.h>
#include <stdio.h>

/*-------------- Macros ------------------*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

//
// SCP MHU configurations
//

static icc_mhu_configuration_t d0_icc_mhu_scp_config[] = {
    // SCP TO AP S Send Response -(TFA response)
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
                .address = SCP_TOP_SCP2MCP_MHU_SND_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_SEND_BASE) + 1,
        .mailbox_size = SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_SEND_SIZE,
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
                .address = SCP_TOP_MCP2SCP_MHU_REC_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_BASE) + 1,
        .mailbox_size = SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE,
    },

};

static icc_mhu_configuration_t d1_icc_mhu_scp_config[] = {
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
                .address = SCP_TOP_SCP2MCP_MHU_SND_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_SEND_BASE) + 1,
        .mailbox_size = SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_SEND_SIZE,
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
                .address = SCP_TOP_MCP2SCP_MHU_REC_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_BASE) + 1,
        .mailbox_size = SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE,
    },
};

//
// MCP MHU configurations
//

static icc_mhu_configuration_t d0_icc_mhu_mcp_config[] = {
    // MCP TO SCP Send Configuration
    {
        .channel_id = MHU_INTERFACE_ID(MCP_LOCAL, SCP_LOCAL),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_STANDARD,
                .direction = ICC_SEND_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MCP_TOP_MCP2SCP_MHU_SND_NS_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_BASE) + 1,
        .mailbox_size = SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE,
    },
    // SCP TO MCP Receive Configuration
    {
        .channel_id = MHU_INTERFACE_ID(SCP_LOCAL, MCP_LOCAL),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_STANDARD,
                .direction = ICC_RECEIVE_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MCP_TOP_MCP2SCP_MHU_REC_NS_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_SEND_BASE) + 1,
        .mailbox_size = SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_SEND_SIZE,
    },
};

static icc_mhu_configuration_t d1_icc_mhu_mcp_config[] = {
    // MCP TO SCP Send Configuration
    {
        .channel_id = MHU_INTERFACE_ID(MCP_LOCAL, SCP_LOCAL),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_STANDARD,
                .direction = ICC_SEND_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MCP_TOP_MCP2SCP_MHU_SND_NS_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_BASE) + 1,
        .mailbox_size = SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE,
    },
    // SCP TO MCP Receive Configuration
    {
        .channel_id = MHU_INTERFACE_ID(SCP_LOCAL, MCP_LOCAL),
        .icc_channel_info =
            {
                .protocol_type = ICC_PROTOCOL_TYPE_STANDARD,
                .direction = ICC_RECEIVE_DIR,

            },
        .channel =
            {
                .type = MHU3_CHANNEL_TYPE_DBCH,
                .address = MCP_TOP_MCP2SCP_MHU_REC_NS_ADDRESS,
                .characteristics = 0,
                .dbch =
                    {
                        .pbx_channel = 0,
                        .pbx_flag_pos = 0,
                        .mbx_channel = 0,
                        .mbx_flag_pos = 0,
                    },
            },
        .mailbox_address = (SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_SEND_BASE) + 1,
        .mailbox_size = SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_SEND_SIZE,
    },
};

//
// Private helper functions
//

static icc_mhu_configuration_t* get_config_table(KNG_DIE_ID die, idsw_cpu_type_t cpu)
{

    icc_mhu_configuration_t* config = NULL;
    if (die == DIE_0)
    {
        if (cpu == CPU_SCP)
        {
            config = d0_icc_mhu_scp_config;
        }
        else {
            config = d0_icc_mhu_mcp_config;
        }
    }
    else {
        if (cpu == CPU_SCP)
        {
            config = d1_icc_mhu_scp_config;
        }
        else {
            config = d1_icc_mhu_mcp_config;
        }
    }

    return config;
}

static uint32_t get_num_configs(KNG_DIE_ID die, idsw_cpu_type_t cpu)
{
    uint32_t num_configs = 0;
    if (die == DIE_0)
    {
        if (cpu == CPU_SCP)
        {
            num_configs = sizeof(d0_icc_mhu_scp_config) / sizeof(icc_mhu_configuration_t);
        }
        else {
            num_configs = sizeof(d0_icc_mhu_mcp_config) / sizeof(icc_mhu_configuration_t);
        }
    }
    else {
        if (cpu == CPU_SCP)
        {
            num_configs = sizeof(d1_icc_mhu_scp_config) / sizeof(icc_mhu_configuration_t);
        }
        else {
            num_configs = sizeof(d1_icc_mhu_mcp_config) / sizeof(icc_mhu_configuration_t);
        }
    }
    return num_configs;
}

/*------------- Functions ----------------*/


/**
 * This is required due to dependencies in the shared ICC MHU library, that requires the
 * entire configuration to be setup. 
 * @TODO: Look into only needing to initialize the MHUs use at each device or interface
 *        initialization or open. 
 */
FPFW_INIT_COMPONENT(mhu_trans, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "atu_svc"))
{
    // Initialize just the primitives for now
    KNG_DIE_ID die_num = idhw_get_die_id();
    idsw_cpu_type_t cpu_id = idsw_get_cpu_type();
    icc_mhu_trans_init(get_config_table(die_num, cpu_id), get_num_configs(die_num, cpu_id));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

//
// This section will initialize each MHU channel (sending and receiving combine) DFWK Device,
// the Interface, and the ICC Base Context for each channel.
// 

FPFW_INIT_COMPONENT(icc_mscp2tfa_if, FPFW_INIT_DEPENDENCIES("dfwk", "mhu_trans"))
{
    /* Setup the device */
    idsw_cpu_type_t cpu_id = idsw_get_cpu_type();

    // We don't configure this path on the MCP. Can configure as needed
    if (cpu_id == CPU_MCP)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    static icc_mhu_transport_device_t s_icc_mhu_dev_mscp_tfa = {};
    icc_mhu_transport_config_t config = {
        .send_core_2_core_id = MHU_INTERFACE_ID(SCP_LOCAL, AP_CORE_SEC),
        .receive_core_2_core_id = MHU_INTERFACE_ID(AP_CORE_SEC, SCP_LOCAL),
        .irq_num = HW_INT_AP2MSCP_MHU_S_INT,
    };

    // Initialize the driver framework device
    fpfw_status_t status = icc_mhu_transport_dfwk_device_init(&s_icc_mhu_dev_mscp_tfa, &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule, &config);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    /* Setup the Interface */
    static icc_mhu_transport_intrf_t s_icc_mhu_intf_mscp_tfa = {};

    status = icc_mhu_trans_dfwk_interface_init(&s_icc_mhu_dev_mscp_tfa, &s_icc_mhu_intf_mscp_tfa);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_icc_mhu_intf_mscp_tfa};
}

FPFW_INIT_COMPONENT(icc_mscp2mscp, FPFW_INIT_DEPENDENCIES("dfwk", "mhu_trans"))
{
    /* Setup the device */
    static icc_mhu_transport_device_t s_icc_mhu_dev_mscp_mscp = {};
    icc_mhu_transport_config_t config = {
        .irq_num = HW_INT_MSCP2MSCP_MHU_INT,
    };

    idsw_cpu_type_t cpu_id = idsw_get_cpu_type();
    if (cpu_id == CPU_SCP)
    {
        config.send_core_2_core_id = MHU_INTERFACE_ID(SCP_LOCAL, MCP_LOCAL);
        config.receive_core_2_core_id = MHU_INTERFACE_ID(MCP_LOCAL, SCP_LOCAL);
    }
    else {
        config.send_core_2_core_id = MHU_INTERFACE_ID(MCP_LOCAL, SCP_LOCAL);
        config.receive_core_2_core_id = MHU_INTERFACE_ID(SCP_LOCAL, MCP_LOCAL);
    }

    // Initialize the driver framework device
    fpfw_status_t status = icc_mhu_transport_dfwk_device_init(&s_icc_mhu_dev_mscp_mscp, &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule, &config);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    /* Setup the Interface */
    static icc_mhu_transport_intrf_t s_icc_mhu_intf_mscp_mscp = {};

    status = icc_mhu_trans_dfwk_interface_init(&s_icc_mhu_dev_mscp_mscp, &s_icc_mhu_intf_mscp_mscp);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    /* Setup the ICC Base Context */
    static uint8_t s_icc_dispatch_buf_mscp_mscp[SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE] = {0};
    static fpfw_icc_base_ctx_t s_icc_base_mscp_mscp_ctx;
    static fpfw_icc_base_config s_icc_base_mscp_mscp_cfg = {
        .transport_interface = &s_icc_mhu_intf_mscp_mscp.header,
        .dispatch_cfg =
            {
                .transport_interface = &s_icc_mhu_intf_mscp_mscp,
                .dispatcher_buffer = &s_icc_dispatch_buf_mscp_mscp,
                .dispatcher_buffer_size = sizeof(s_icc_dispatch_buf_mscp_mscp),
                .strategy =
                    {
                        .cmd_code =
                            {
                                .is_used = true,
                                .start_pos = ICC_MHU_CMD_BIT_OFFSET,
                                .size_bits = ICC_MHU_CMD_SIZE_BITS - 1,
                                .valid_max = UINT32_MAX / 2,
                                .valid_min = 0,
                            },
                        .seq_num =
                            {
                                .is_used = false,
                                .start_pos = ICC_MHU_TOKEN_BIT_OFFSET,
                                .size_bits = ICC_MHU_TOKEN_SIZE_BITS - 1,
                                .valid_max = UINT32_MAX / 2,
                                .valid_min = 0,
                            },
                    },
                .match_strategy_cb = icc_mhu_trans_dwfk_icc_dispatcher_match_cb,
                .match_strategy_ctx = NULL,
            },
        .ctx = NULL,
    };

    // Initialize ICC base ctx for hsp mailbox transport driver
    status = fpfw_icc_base_init(&s_icc_base_mscp_mscp_ctx, &s_icc_base_mscp_mscp_cfg);

    if (status == FPFW_STATUS_SUCCESS)
    {
        // start the dispatcher to receive data over hsp mailbox
        status = fpfw_icc_dispatcher_start(&s_icc_base_mscp_mscp_ctx.dispatch_ctx);
        if (status != FPFW_ICC_DISPATCH_STATUS_SUCCESS)
        {
            return (fpfw_init_result_t){status, NULL};
        }
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_icc_base_mscp_mscp_ctx};
}
