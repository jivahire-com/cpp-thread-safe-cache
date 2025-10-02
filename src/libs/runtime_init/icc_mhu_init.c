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
#include <FpFwUtils.h>
#include <atu_api.h>
#include <fpfw_icc_base.h>
#include <fpfw_icc_base_i.h>
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <kng_icc_shared.h>
#include <kng_scmi_shared.h>
#include <kng_scp_tfa_shared.h>
#include <kng_soc_constants.h>
#include <limits.h>
#include <mhu_icc_payloads.h>
#include <mhu_icc_transport.h>
#include <mscp_exp_rmss_memory_map.h>
#include <silibs_mcp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdint.h>
#include <stdio.h>
#include <system_info.h>

/*-------------- Macros ------------------*/

#define SMT_CHANNEL_FREE (1)
#define ASYNC_SEND_RETRY_PERIOD_NS \
    (FPFW_DUR_MS(10)) // Current implementation of the timer has a minimum period of 10ms
#define ASYNC_SEND_RETRY_MAX (5)

// ICC Base doesn't have an actual timeout on sync messages, and instead
// it loops until the sync retry count is reached. We use a large retry
// count here to avoid sync send failures as much as possible without
// looping forever in ICC Base. Failures can still occur and ICC Base
// returns an error code on such failures.
#define MAX_SYNC_RETRY_COUNT (50)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

//
// Initialize each MHU channel (sending and receiving combine) DFWK Device,
// the Interface, and the ICC Base Context for each channel.
//

#ifdef SCP_RUNTIME_INIT
FPFW_INIT_COMPONENT(icc_mscp2tfa_if,
                    FPFW_INIT_DEPENDENCIES("dfwk", "atu_svc", "hw_ver", "mesh_stg_2", "shared_mem", "sysinfo"))
#else
FPFW_INIT_COMPONENT(icc_mscp2tfa_if, FPFW_INIT_DEPENDENCIES("dfwk", "atu_svc", "hw_ver", "sysinfo"))
#endif
{

    KNG_CPU_TYPE cpu_id = idsw_get_cpu_type();

    // This component is not needed on MCP. Create the init node anyways to follow existing icc
    // init patterns.
    if (cpu_id == CPU_MCP)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    static mhu_icc_transport_device_t s_icc_mscp_2_tfa_dev = {};

    uintptr_t recv_payload_address =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(D0_ARSM_TFA_AP_2_SCP_SEND_BASE);
    uintptr_t send_payload_address =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(D0_ARSM_TFA_AP_2_SCP_RECEIVE_BASE);
    KNG_DIE_ID die_id = idsw_get_die_id();
    if (die_id == DIE_1)
    {
        recv_payload_address =
            MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D1_ARSM_TFA_AP_2_SCP_SEND_BASE);
        send_payload_address =
            MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D1_ARSM_TFA_AP_2_SCP_RECEIVE_BASE);
    }

    mhu_icc_transport_device_config_t dev_config = {
        .recv_reset_on_init = !system_info_is_warm_start(),
        .recv_irq_num = HW_INT_AP2MSCP_MHU_S_INT,
        .recv_channel =
            {
                .mhu_addr = MHU_SCP_AP_S_REC_BASE_ADDRESS,
                .ch_id = MHU_INTERFACE_ID(AP_CORE_SEC, SCP_LOCAL),
                .ch_shared_mem_cacheable = false,
                .ch_shared_mem_size = D0_ARSM_TFA_AP_2_SCP_SEND_SIZE,
                .ch_shared_mem_addr = recv_payload_address,
                .ch_db_config =
                    {
                        .channel_num = 0,
                        .flag_num = 0,
                    },
            },
        .send_channel =
            {
                .mhu_addr = MHU_SCP_AP_S_SEND_BASE_ADDRESS,
                .ch_id = MHU_INTERFACE_ID(SCP_LOCAL, AP_CORE_SEC),
                .ch_shared_mem_cacheable = false,
                .ch_shared_mem_size = D0_ARSM_TFA_AP_2_SCP_SEND_SIZE,
                .ch_shared_mem_addr = send_payload_address,
                .ch_db_config =
                    {
                        .channel_num = 0,
                        .flag_num = 0,
                    },
            },
        .async_send_retry_period = ASYNC_SEND_RETRY_PERIOD_NS,
        .async_send_retry_max = ASYNC_SEND_RETRY_MAX,
    };

    // Initialize the driver framework device
    fpfw_status_t status = mhu_icc_transport_device_init(&s_icc_mscp_2_tfa_dev,
                                                         &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule,
                                                         &dev_config);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    /* Setup the Interface */
    static mhu_icc_transport_intrf_t s_icc_mscp_2_tfa_if = {};

    status = mhu_icc_transport_interface_init(&s_icc_mscp_2_tfa_dev, &s_icc_mscp_2_tfa_if);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    int32_t dwfk_status = DfwkClientInterfaceOpen(&(s_icc_mscp_2_tfa_if.base_interface));
    if (DFWK_FAILED(dwfk_status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_icc_mscp_2_tfa_if};
}
//
// RT MailBox ICC
//
#ifdef SCP_RUNTIME_INIT
FPFW_INIT_COMPONENT(icc_mscp2aprt, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "atu_svc", "mesh_stg_2", "sysinfo"))
#else
FPFW_INIT_COMPONENT(icc_mscp2aprt, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "atu_svc", "sysinfo"))
#endif
{
    idsw_die_id_t die_id = idsw_get_die_id();

    KNG_CPU_TYPE cpu_id = idsw_get_cpu_type();

    // This component is not needed on MCP. Create the init node anyways to follow existing icc
    // init patterns.
    if (cpu_id == CPU_MCP)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    static mhu_icc_transport_device_t s_icc_mscp_2_ap_rt_dev = {};

    uintptr_t recv_payload_address =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(D0_ARSM_TFA_AP_2_SCP_RT_SEND_BASE);
    uintptr_t send_payload_address =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(D0_ARSM_TFA_AP_2_SCP_RT_RECEIVE_BASE);

    uint32_t recv_ch_id = MHU_INTERFACE_ID(AP_CORE_RT, SCP_LOCAL);
    uint32_t send_ch_id = MHU_INTERFACE_ID(SCP_LOCAL, AP_CORE_RT);

    if (die_id == DIE_1)
    {
        recv_payload_address =
            MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D1_ARSM_TFA_AP_2_SCP_RT_SEND_BASE);
        send_payload_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR +
                               ARSM_GET_REGION_OFFSET(D1_ARSM_TFA_AP_2_SCP_RT_RECEIVE_BASE);
    }

    mhu_icc_transport_device_config_t dev_config = {
        .recv_reset_on_init = system_info_is_warm_start(),
        .recv_irq_num = HW_INT_AP2MSCP_MHU_RT_INT,
        .recv_channel =
            {
                .mhu_addr = MHU_SCP_AP_RT_REC_BASE_ADDRESS,
                .ch_id = recv_ch_id,
                .ch_shared_mem_cacheable = false,
                .ch_shared_mem_size = D0_ARSM_TFA_AP_2_SCP_RT_RECEIVE_SIZE,
                .ch_shared_mem_addr = recv_payload_address,
                .ch_db_config =
                    {
                        .channel_num = 0,
                        .flag_num = 0,
                    },
            },
        .send_channel =
            {
                .mhu_addr = MHU_SCP_AP_RT_SEND_BASE_ADDRESS,
                .ch_id = send_ch_id,
                .ch_shared_mem_cacheable = false,
                .ch_shared_mem_size = D0_ARSM_TFA_AP_2_SCP_RT_SEND_SIZE,
                .ch_shared_mem_addr = send_payload_address,
                .ch_db_config =
                    {
                        .channel_num = 0,
                        .flag_num = 0,
                    },
            },
        .async_send_retry_period = ASYNC_SEND_RETRY_PERIOD_NS,
        .async_send_retry_max = ASYNC_SEND_RETRY_MAX,
    };

    // Initialize the driver framework device
    fpfw_status_t status = mhu_icc_transport_device_init(&s_icc_mscp_2_ap_rt_dev,
                                                         &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule,
                                                         &dev_config);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    /* Setup the Interface */
    static mhu_icc_transport_intrf_t s_icc_mscp_2_ap_rt_if = {};

    status = mhu_icc_transport_interface_init(&s_icc_mscp_2_ap_rt_dev, &s_icc_mscp_2_ap_rt_if);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    /* Setup the ICC Base Context */
    static uint8_t s_icc_dispatch_buf_mscp_ap_rt[D0_ARSM_TFA_AP_2_SCP_RT_SEND_SIZE] = {0};
    static fpfw_icc_base_ctx_t s_icc_base_mscp_ap_rt_ctx;
    static fpfw_icc_base_config s_icc_base_mscp_ap_rt_cfg = {
        .transport_interface = &s_icc_mscp_2_ap_rt_if.base_interface,
        .dispatch_cfg =
            {
                .transport_interface = &s_icc_mscp_2_ap_rt_if,
                .dispatcher_buffer = &s_icc_dispatch_buf_mscp_ap_rt,
                .dispatcher_buffer_size = sizeof(s_icc_dispatch_buf_mscp_ap_rt),
                .strategy =
                    {
                        .cmd_code =
                            {
                                .is_used = true,
                                .start_pos = ICC_MHU_CMD_BIT_OFFSET,
                                .size_bits = ICC_MHU_CMD_SIZE_BITS - 1,
                                .valid_max = UINT32_MAX >> 1,
                                .valid_min = 0,
                            },
                        .seq_num =
                            {
                                .is_used = true,
                                .start_pos = ICC_MHU_TOKEN_BIT_OFFSET,
                                .size_bits = ICC_MHU_TOKEN_SIZE_BITS - 1,
                                .valid_max = UINT32_MAX >> 1,
                                .valid_min = 0,
                            },
                    },
                .match_strategy_cb = mhu_icc_transport_dispatcher_match_cb,
                .match_strategy_ctx = NULL,
            },
        .ctx = NULL,
        .sync_loop_count = MAX_SYNC_RETRY_COUNT,
    };

    // Initialize ICC base ctx
    status = fpfw_icc_base_init(&s_icc_base_mscp_ap_rt_ctx, &s_icc_base_mscp_ap_rt_cfg);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    // start the dispatcher to receive data
    status = fpfw_icc_dispatcher_start(&s_icc_base_mscp_ap_rt_ctx.dispatch_ctx);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_icc_base_mscp_ap_rt_ctx};
}

FPFW_INIT_COMPONENT(icc_mscp2mscp, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "sysinfo"))
{
    KNG_CPU_TYPE cpu_id = idsw_get_cpu_type();

    static mhu_icc_transport_device_t s_icc_mscp_2_mscp_dev = {};

    uintptr_t recv_payload_address = (SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_BASE);
    uintptr_t send_payload_address = (SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_SEND_BASE);
    uintptr_t recv_mhu_addr = SCP_TOP_MCP2SCP_MHU_REC_ADDRESS;
    uintptr_t send_mhu_addr = SCP_TOP_SCP2MCP_MHU_SND_ADDRESS;
    uint32_t recv_ch_id = MHU_INTERFACE_ID(MCP_LOCAL, SCP_LOCAL);
    uint32_t send_ch_id = MHU_INTERFACE_ID(SCP_LOCAL, MCP_LOCAL);
    if (cpu_id == CPU_MCP)
    {
        recv_payload_address = (SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_SEND_BASE);
        send_payload_address = (SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_BASE);
        recv_mhu_addr = MCP_TOP_MCP2SCP_MHU_REC_NS_ADDRESS;
        send_mhu_addr = MCP_TOP_MCP2SCP_MHU_SND_NS_ADDRESS;
        recv_ch_id = MHU_INTERFACE_ID(SCP_LOCAL, MCP_LOCAL);
        send_ch_id = MHU_INTERFACE_ID(MCP_LOCAL, SCP_LOCAL);
    }

    mhu_icc_transport_device_config_t dev_config = {
        .recv_reset_on_init = system_info_is_warm_start(),
        .recv_irq_num = HW_INT_MSCP2MSCP_MHU_INT,
        // The RMSS MSCP_EXP RAM block where the payloads are stored is cacheable.
        .recv_channel =
            {
                .mhu_addr = recv_mhu_addr,
                .ch_id = recv_ch_id,
                .ch_shared_mem_cacheable = true,
                .ch_shared_mem_size = SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_SEND_SIZE,
                .ch_shared_mem_addr = recv_payload_address,
                .ch_db_config =
                    {
                        .channel_num = 0,
                        .flag_num = 0,
                    },
            },
        .send_channel =
            {
                .mhu_addr = send_mhu_addr,
                .ch_id = send_ch_id,
                .ch_shared_mem_cacheable = true,
                .ch_shared_mem_size = SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_SEND_SIZE,
                .ch_shared_mem_addr = send_payload_address,
                .ch_db_config =
                    {
                        .channel_num = 0,
                        .flag_num = 0,
                    },
            },
        .async_send_retry_period = ASYNC_SEND_RETRY_PERIOD_NS,
        .async_send_retry_max = ASYNC_SEND_RETRY_MAX,
    };

    // Initialize the driver framework device
    fpfw_status_t status = mhu_icc_transport_device_init(&s_icc_mscp_2_mscp_dev,
                                                         &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule,
                                                         &dev_config);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    /* Setup the Interface */
    static mhu_icc_transport_intrf_t s_icc_mscp_2_mscp_if = {};

    status = mhu_icc_transport_interface_init(&s_icc_mscp_2_mscp_dev, &s_icc_mscp_2_mscp_if);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    /* Setup the ICC Base Context */
    static uint8_t s_icc_dispatch_buf_mscp_mscp[SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE] = {0};
    static fpfw_icc_base_ctx_t s_icc_base_mscp_mscp_ctx;
    static fpfw_icc_base_config s_icc_base_mscp_mscp_cfg = {
        .transport_interface = &s_icc_mscp_2_mscp_if.base_interface,
        .dispatch_cfg =
            {
                .transport_interface = &s_icc_mscp_2_mscp_if,
                .dispatcher_buffer = &s_icc_dispatch_buf_mscp_mscp,
                .dispatcher_buffer_size = sizeof(s_icc_dispatch_buf_mscp_mscp),
                .strategy =
                    {
                        .cmd_code =
                            {
                                .is_used = true,
                                .start_pos = ICC_MHU_CMD_BIT_OFFSET,
                                .size_bits = ICC_MHU_CMD_SIZE_BITS - 1,
                                .valid_max = UINT32_MAX >> 1,
                                .valid_min = 0,
                            },
                        .seq_num =
                            {
                                .is_used = true,
                                .start_pos = ICC_MHU_TOKEN_BIT_OFFSET,
                                .size_bits = ICC_MHU_TOKEN_SIZE_BITS - 1,
                                .valid_max = UINT32_MAX >> 1,
                                .valid_min = 0,
                            },
                    },
                .match_strategy_cb = mhu_icc_transport_dispatcher_match_cb,
                .match_strategy_ctx = NULL,
            },
        .ctx = NULL,
        .sync_loop_count = MAX_SYNC_RETRY_COUNT,
    };

    // Initialize ICC base ctx
    status = fpfw_icc_base_init(&s_icc_base_mscp_mscp_ctx, &s_icc_base_mscp_mscp_cfg);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    // start the dispatcher to receive data
    status = fpfw_icc_dispatcher_start(&s_icc_base_mscp_mscp_ctx.dispatch_ctx);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_icc_base_mscp_mscp_ctx};
}

#ifdef SCP_RUNTIME_INIT
FPFW_INIT_COMPONENT(icc_mscp2apns, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "atu_svc", "mesh_stg_2", "sysinfo"))
#else
FPFW_INIT_COMPONENT(icc_mscp2apns, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "atu_svc", "sysinfo"))
#endif
{
    KNG_CPU_TYPE cpu_id = idsw_get_cpu_type();
    idsw_die_id_t die_id = idsw_get_die_id();

    static mhu_icc_transport_device_t s_icc_mscp_2_ap_ns_dev = {};

    uintptr_t recv_payload_address = (ICC_MHU_PAYLOADS_D0_AP_NS_TO_SCP_AP_WINDOW_BASE);
    uintptr_t send_payload_address = (ICC_MHU_PAYLOADS_D0_SCP_TO_AP_NS_AP_WINDOW_BASE);
    uintptr_t recv_mhu_addr = SCP_TOP_AP2SCP_MHU_REC_NS_ADDRESS;
    uintptr_t send_mhu_addr = SCP_TOP_SCP2AP_MHU_SND_NS_ADDRESS;
    uint32_t recv_ch_id = MHU_INTERFACE_ID(AP_CORE_NSEC, SCP_LOCAL);
    uint32_t send_ch_id = MHU_INTERFACE_ID(SCP_LOCAL, AP_CORE_NSEC);

    if (die_id == DIE_1)
    {
        recv_payload_address = (ICC_MHU_PAYLOADS_D1_AP_NS_TO_SCP_AP_WINDOW_BASE);
        send_payload_address = (ICC_MHU_PAYLOADS_D1_SCP_TO_AP_NS_AP_WINDOW_BASE);
    }
    if (cpu_id == CPU_MCP)
    {
        recv_payload_address = (ICC_MHU_PAYLOADS_D0_AP_NS_TO_MCP_AP_WINDOW_BASE);
        send_payload_address = (ICC_MHU_PAYLOADS_D0_MCP_TO_AP_NS_AP_WINDOW_BASE);
        recv_mhu_addr = MCP_TOP_MCP2AP_MHU_REC_NS_ADDRESS;
        send_mhu_addr = MCP_TOP_MCP2AP_MHU_SND_NS_ADDRESS;
        recv_ch_id = MHU_INTERFACE_ID(AP_CORE_NSEC, MCP_LOCAL);
        send_ch_id = MHU_INTERFACE_ID(MCP_LOCAL, AP_CORE_NSEC);
        if (die_id == DIE_1)
        {
            recv_payload_address = (ICC_MHU_PAYLOADS_D1_AP_NS_TO_MCP_AP_WINDOW_BASE);
            send_payload_address = (ICC_MHU_PAYLOADS_D1_MCP_TO_AP_NS_AP_WINDOW_BASE);
        }
    }

    mhu_icc_transport_device_config_t dev_config = {
        .recv_reset_on_init = system_info_is_warm_start(),
        .recv_irq_num = HW_INT_MSCP2AP_NS_MHU_INT,
        .recv_channel =
            {
                .mhu_addr = recv_mhu_addr,
                .ch_id = recv_ch_id,
                .ch_shared_mem_cacheable = false,
                .ch_shared_mem_size = ICC_MHU_DDR_PAYLOAD_SIZE,
                .ch_shared_mem_addr = recv_payload_address,
                .ch_db_config =
                    {
                        .channel_num = 0,
                        .flag_num = 0,
                    },
            },
        .send_channel =
            {
                .mhu_addr = send_mhu_addr,
                .ch_id = send_ch_id,
                .ch_shared_mem_cacheable = false,
                .ch_shared_mem_size = ICC_MHU_DDR_PAYLOAD_SIZE,
                .ch_shared_mem_addr = send_payload_address,
                .ch_db_config =
                    {
                        .channel_num = 0,
                        .flag_num = 0,
                    },
            },
        .async_send_retry_period = ASYNC_SEND_RETRY_PERIOD_NS,
        .async_send_retry_max = ASYNC_SEND_RETRY_MAX,
    };

    // Initialize the driver framework device
    fpfw_status_t status = mhu_icc_transport_device_init(&s_icc_mscp_2_ap_ns_dev,
                                                         &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule,
                                                         &dev_config);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    /* Setup the Interface */
    static mhu_icc_transport_intrf_t s_icc_mscp_2_ap_ns_if = {};

    status = mhu_icc_transport_interface_init(&s_icc_mscp_2_ap_ns_dev, &s_icc_mscp_2_ap_ns_if);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    /* Setup the ICC Base Context */
    static uint8_t s_icc_dispatch_buf_mscp_ap_ns[ICC_MHU_DDR_PAYLOAD_SIZE] = {0};
    static fpfw_icc_base_ctx_t s_icc_base_mscp_ap_ns_ctx;
    static fpfw_icc_base_config s_icc_base_mscp_ap_ns_cfg = {
        .transport_interface = &s_icc_mscp_2_ap_ns_if.base_interface,
        .dispatch_cfg =
            {
                .transport_interface = &s_icc_mscp_2_ap_ns_if,
                .dispatcher_buffer = &s_icc_dispatch_buf_mscp_ap_ns,
                .dispatcher_buffer_size = sizeof(s_icc_dispatch_buf_mscp_ap_ns),
                .strategy =
                    {
                        .cmd_code =
                            {
                                .is_used = true,
                                .start_pos = ICC_MHU_CMD_BIT_OFFSET,
                                .size_bits = ICC_MHU_CMD_SIZE_BITS - 1,
                                .valid_max = UINT32_MAX >> 1,
                                .valid_min = 0,
                            },
                        .seq_num =
                            {
                                .is_used = true,
                                .start_pos = ICC_MHU_TOKEN_BIT_OFFSET,
                                .size_bits = ICC_MHU_TOKEN_SIZE_BITS - 1,
                                .valid_max = UINT32_MAX >> 1,
                                .valid_min = 0,
                            },
                    },
                .match_strategy_cb = mhu_icc_transport_dispatcher_match_cb,
                .match_strategy_ctx = NULL,
            },
        .ctx = NULL,
        .sync_loop_count = MAX_SYNC_RETRY_COUNT,
    };

    // Initialize ICC base ctx
    status = fpfw_icc_base_init(&s_icc_base_mscp_ap_ns_ctx, &s_icc_base_mscp_ap_ns_cfg);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    // start the dispatcher to receive data
    status = fpfw_icc_dispatcher_start(&s_icc_base_mscp_ap_ns_ctx.dispatch_ctx);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_icc_base_mscp_ap_ns_ctx};
}

#ifdef SCP_RUNTIME_INIT
FPFW_INIT_COMPONENT(icc_die2die, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "atu_svc", "mesh_stg_2", "sysinfo"))
#else
FPFW_INIT_COMPONENT(icc_die2die, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "atu_svc", "sysinfo"))
#endif
{
    bool single_die = idhw_is_single_die_boot_en();

    // We don't have these channels on single die systems
    if (single_die)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    KNG_CPU_TYPE cpu_id = idsw_get_cpu_type();
    idsw_die_id_t die_id = idsw_get_die_id();

    static mhu_icc_transport_device_t s_icc_die_2_die_dev = {};

    uintptr_t recv_payload_address = (ICC_MHU_PAYLOADS_D1_SCP_TO_D0_SCP_AP_WINDOW_BASE);
    uintptr_t send_payload_address = (ICC_MHU_PAYLOADS_D0_SCP_TO_D1_SCP_AP_WINDOW_BASE);
    uintptr_t recv_mhu_addr = SCP_TOP_REM_SCP2LOC_SCP_MHU_REC_ADDRESS;
    uintptr_t send_mhu_addr = MSCP_ATU_AP_WINDOW_ICC_MHU_SCP2SCP_SEND_FRAME_BASE_ADDR;
    uint32_t recv_ch_id = MHU_INTERFACE_ID(SCP_REMOTE, SCP_LOCAL);
    uint32_t send_ch_id = MHU_INTERFACE_ID(SCP_LOCAL, SCP_REMOTE);

    if (die_id == DIE_1)
    {
        recv_payload_address = (ICC_MHU_PAYLOADS_D0_SCP_TO_D1_SCP_AP_WINDOW_BASE);
        send_payload_address = (ICC_MHU_PAYLOADS_D1_SCP_TO_D0_SCP_AP_WINDOW_BASE);
    }
    if (cpu_id == CPU_MCP)
    {
        recv_payload_address = (ICC_MHU_PAYLOADS_D1_MCP_TO_D0_MCP_AP_WINDOW_BASE);
        send_payload_address = (ICC_MHU_PAYLOADS_D0_MCP_TO_D1_MCP_AP_WINDOW_BASE);
        recv_mhu_addr = MCP_TOP_REM_MCP2LOC_MCP_MHU_RCV_ADDRESS;
        send_mhu_addr = MSCP_ATU_AP_WINDOW_ICC_MHU_MCP2MCP_SEND_FRAME_BASE_ADDR;
        recv_ch_id = MHU_INTERFACE_ID(MCP_REMOTE, MCP_LOCAL);
        send_ch_id = MHU_INTERFACE_ID(MCP_LOCAL, MCP_REMOTE);
        if (die_id == DIE_1)
        {
            recv_payload_address = (ICC_MHU_PAYLOADS_D0_MCP_TO_D1_MCP_AP_WINDOW_BASE);
            send_payload_address = (ICC_MHU_PAYLOADS_D1_MCP_TO_D0_MCP_AP_WINDOW_BASE);
        }
    }

    mhu_icc_transport_device_config_t dev_config = {
        .recv_reset_on_init = system_info_is_warm_start(),
        .recv_irq_num = HW_INT_D2D_NS_INT,
        .recv_channel =
            {
                .mhu_addr = recv_mhu_addr,
                .ch_id = recv_ch_id,
                .ch_shared_mem_cacheable = false,
                .ch_shared_mem_size = ICC_MHU_DDR_PAYLOAD_SIZE,
                .ch_shared_mem_addr = recv_payload_address,
                .ch_db_config =
                    {
                        .channel_num = 0,
                        .flag_num = 0,
                    },
            },
        .send_channel =
            {
                .mhu_addr = send_mhu_addr,
                .ch_id = send_ch_id,
                .ch_shared_mem_cacheable = false,
                .ch_shared_mem_size = ICC_MHU_DDR_PAYLOAD_SIZE,
                .ch_shared_mem_addr = send_payload_address,
                .ch_db_config =
                    {
                        .channel_num = 0,
                        .flag_num = 0,
                    },
            },
        .async_send_retry_period = ASYNC_SEND_RETRY_PERIOD_NS,
        .async_send_retry_max = ASYNC_SEND_RETRY_MAX,
    };

    // Initialize the driver framework device
    fpfw_status_t status = mhu_icc_transport_device_init(&s_icc_die_2_die_dev,
                                                         &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule,
                                                         &dev_config);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    /* Setup the Interface */
    static mhu_icc_transport_intrf_t s_icc_die_2_die_if = {};

    status = mhu_icc_transport_interface_init(&s_icc_die_2_die_dev, &s_icc_die_2_die_if);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    /* Setup the ICC Base Context */
    static uint8_t s_icc_dispatch_buf_die_2_die[ICC_MHU_DDR_PAYLOAD_SIZE] = {0};
    static fpfw_icc_base_ctx_t s_icc_base_die_2_die_ctx;
    static fpfw_icc_base_config s_icc_base_die_2_die_cfg = {
        .transport_interface = &s_icc_die_2_die_if.base_interface,
        .dispatch_cfg =
            {
                .transport_interface = &s_icc_die_2_die_if,
                .dispatcher_buffer = &s_icc_dispatch_buf_die_2_die,
                .dispatcher_buffer_size = sizeof(s_icc_dispatch_buf_die_2_die),
                .strategy =
                    {
                        .cmd_code =
                            {
                                .is_used = true,
                                .start_pos = ICC_MHU_CMD_BIT_OFFSET,
                                .size_bits = ICC_MHU_CMD_SIZE_BITS - 1,
                                .valid_max = UINT32_MAX >> 1,
                                .valid_min = 0,
                            },
                        .seq_num =
                            {
                                .is_used = true,
                                .start_pos = ICC_MHU_TOKEN_BIT_OFFSET,
                                .size_bits = ICC_MHU_TOKEN_SIZE_BITS - 1,
                                .valid_max = UINT32_MAX >> 1,
                                .valid_min = 0,
                            },
                    },
                .match_strategy_cb = mhu_icc_transport_dispatcher_match_cb,
                .match_strategy_ctx = NULL,
            },
        .ctx = NULL,
        .sync_loop_count = MAX_SYNC_RETRY_COUNT,
    };

    // Initialize ICC base ctx for hsp mailbox transport driver
    status = fpfw_icc_base_init(&s_icc_base_die_2_die_ctx, &s_icc_base_die_2_die_cfg);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    // start the dispatcher to receive data over hsp mailbox
    status = fpfw_icc_dispatcher_start(&s_icc_base_die_2_die_ctx.dispatch_ctx);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_icc_base_die_2_die_ctx};
}
