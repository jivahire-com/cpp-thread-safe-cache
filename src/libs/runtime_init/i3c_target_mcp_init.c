//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file i3c_target_mcp_init.c
 * Initialize i3c_2 mcp target controller
 */
/*------------- Includes -----------------*/
#include <DfwkThreadXHost.h>
#include <fpfw_dw_i3c.h> // for fpfw_dw_i3c_config_t, fpfw_dw_i3c_device_t
#include <fpfw_init.h>   // for FPFW_INIT_COMPONENT, FPFW_INIT_D...
#include <interrupts.h>
#include <mcp_exp_top_regs.h>
#include <silibs_mcp_top_regs.h>
/*------------- Typedefs -----------------*/
/* It's programmed into the i3c hw as the static address, but in practice BMC doesn't use it,
   BMC used to do SETDASA to assign DA, and that have used the static address */
#define TARGET_ADDRESS (0xDE)

/* Microsoft MIPI vendor ID is 0x02CB https://mid.mipi.org/  */
#define MSFT_MFR_ID     (0x02CB)
#define MSFT_TARGET_PID 0x12345678
/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint32_t s_TargetResponseBuf[8];
static uint32_t s_TargetRxDataBuf[128];
/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(i3c_target, FPFW_INIT_DEPENDENCIES("hw_ver", "nvic", "dfwk", "gpio_lib"))
{

    DEBUG_PRINT("I3C target init \n");

    static fpfw_dw_i3c_device_t s_I3cTargetDevice;

    static fpfw_dw_i3c_config_t s_I3cTargetConfig = {
        .dw_i3c_config.register_base_addr = MCP_TOP_MCP_EXP_ADDRESS + MSCP_EXP_TOP_I3C_2_ADDRESS,
        .dw_i3c_config.address = TARGET_ADDRESS,
        .dw_i3c_config.irq_num = HW_INT_I3C_CTRL_2_INT,
        .dw_i3c_config.index = 2,
        .dw_i3c_config.slv_mfg_id = MSFT_MFR_ID,
        .dw_i3c_config.slv_pid = MSFT_TARGET_PID,
        .dw_i3c_config.slv_dcr = 0xC3, /* 0xC3 fpr I3C and 0xCC for MCTP  */
        .dw_i3c_config.slv_rx_resp_buf = (uintptr_t)s_TargetResponseBuf,
        .dw_i3c_config.slv_rx_resp_buf_len = sizeof(s_TargetResponseBuf),
        .dw_i3c_config.slv_rx_data_buf = (uintptr_t)s_TargetRxDataBuf,
        .dw_i3c_config.slv_rx_data_buf_len = sizeof(s_TargetRxDataBuf),
        .device_name = "I3C_2 Target Device"};

    fpfw_status_t status = fpfw_dw_i3c_initialize(&s_I3cTargetDevice, &s_I3cTargetConfig);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    DfwkDeviceInitialize(&s_I3cTargetDevice.header, &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_I3cTargetDevice};
}