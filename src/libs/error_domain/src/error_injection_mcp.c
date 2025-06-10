//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_injection_mcp.c
 * This file contains mcp ras injection related functionality.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <arm_intrinsic.h> // for __DSB on Windows builds (empty define)
#include <bug_check.h>
#include <cper.h>
#include <error_domain_i.h>
#include <health_monitor_icc.h>
#include <mcp_exp_top_regs.h>
#include <mscp_error_domain.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_ras_and_init_ctrl_registers_regs.h>
#include <nvic.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <mcp_top_regs.h>

// clang-format off
#include <cmsis_m7.h>
#ifdef PLATFORM_CACHING_ENABLED
#include <cmsis_compiler.h>
#include <m-profile/armv7m_cachel1.h>
#endif
// clang-format on

/*-- Symbolic Constant Macros (defines) --*/
#define SCF_RAM_ADDRESS   (MCP_TOP_SCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_RAM_ADDRESS)
#define RMSS_RAM0_ADDRESS (MCP_TOP_SCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS)
#define RMSS_RAM1_ADDRESS (MCP_TOP_SCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS)
#define DTC_RAM_ADDRESS   (MCP_TOP_MCP_DATA_RAM_ADDRESS)

#define MCP_EXP_TOP_RAM0_BASE (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS)
// // CACHEABLE SECTION 1856KB
#define MCP_EXP_CACHEABLE_SIZE         (1856 * SL_1KB)
#define MCP_EXP_CACHEABLE_SECTION_BASE (MCP_EXP_TOP_RAM0_BASE)
#define MCP_EXP_CACHEABLE_SECTION_END  (MCP_EXP_CACHEABLE_SECTION_BASE + MCP_EXP_CACHEABLE_SIZE - 1)

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static fpfw_icc_base_ctx_t* mhu_handle_read = NULL;
static vptr_mscp_ras_and_init_ctrl_registers_reg mcp_ras_and_init_ctrl_registers_reg =
    (vptr_mscp_ras_and_init_ctrl_registers_reg)(MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS);

/*-------------- Functions ---------------*/

static void inject_err_by_access(uint32_t addr)
{
    __DSB();

    if (MCP_EXP_CACHEABLE_SECTION_BASE <= addr && addr <= MCP_EXP_CACHEABLE_SECTION_END)
    {
        SCB_InvalidateDCache_by_Addr((uint32_t*)addr, sizeof(uint32_t));
    }

    MMIO_READ32(addr);
}

static void mcp_error_injection_request_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    if (status == FPFW_STATUS_SUCCESS)
    {
        hm_mhu_error_injection_payload_t* hm_err_injection_payload = (hm_mhu_error_injection_payload_t*)context;

        if (hm_err_injection_payload != NULL && hm_err_injection_payload->header.msg_header.command == ICC_HM_ERROR_INJECTION_MCP)
        {
            // TODO: (https://azurecsi.visualstudio.com/Dev/_workitems/edit/2583683/?view=edit)
            // Port SCP RAS handling to MCP (part 2) for error types with missing errctrl_reg
            // TODO: (https://azurecsi.visualstudio.com/Dev/_workitems/edit/2583683/)
            // Port SCP RAS handling to MCP for RSM, M7, etc.
            switch (hm_err_injection_payload->error_injection_info.param_type.error_type)
            {
            case MCP_ERROR_TYPE_TCM_CE:
                MMIO_UPDATE32(&mcp_ras_and_init_ctrl_registers_reg->tcmecc_errctrl.as_uint32,
                              MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_TGT_RAM_MASK |
                                  MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_INJECT_ERROR_MASK,
                              TCM_TGT_RAM_DTCM0RAM | MASK_CE);
                inject_err_by_access(DTC_RAM_ADDRESS);
                break;
            case MCP_ERROR_TYPE_TCM_UE:
                MMIO_UPDATE32(&mcp_ras_and_init_ctrl_registers_reg->tcmecc_errctrl.as_uint32,
                              MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_TGT_RAM_MASK |
                                  MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_INJECT_ERROR_MASK,
                              TCM_TGT_RAM_DTCM0RAM | MASK_UE);
                inject_err_by_access(DTC_RAM_ADDRESS);
                break;
            case MCP_ERROR_TYPE_TCM_OVERFLOW:
                nvic_global_disable();
                MMIO_UPDATE32(&mcp_ras_and_init_ctrl_registers_reg->tcmecc_errctrl.as_uint32,
                              MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_TGT_RAM_MASK |
                                  MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_INJECT_ERROR_MASK,
                              TCM_TGT_RAM_DTCM0RAM | MASK_CE);
                inject_err_by_access(DTC_RAM_ADDRESS);
                MMIO_UPDATE32(&mcp_ras_and_init_ctrl_registers_reg->tcmecc_errctrl.as_uint32,
                              MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_TGT_RAM_MASK |
                                  MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_INJECT_ERROR_MASK,
                              TCM_TGT_RAM_DTCM0RAM | MASK_CE);
                inject_err_by_access(DTC_RAM_ADDRESS);
                nvic_global_enable();
                break;
            case MCP_ERROR_TYPE_HARD_FAULT:
                trigger_hard_fault();
                break;
            case MCP_ERROR_TYPE_BUS_FAULT:
                trigger_bus_fault();
                break;
            case MCP_ERROR_TYPE_MMU_FAULT:
                trigger_mmu_fault();
                break;
            case MCP_ERROR_TYPE_USAGE_FAULT:
            case MCP_ERROR_TYPE_FW_FAULT:
                trigger_usage_fault();
                break;
            case MCP_ERROR_TYPE_WATCHDOG:
                trigger_mscp_watchdog_fault();
                break;

            default:
                FPFW_DBGPRINT_ERROR("Invalid/Unsupported MCP error type(%d)\n",
                                    hm_err_injection_payload->error_injection_info.param_type.error_type);
                break;
            }
        }
    }

    mhu_handle_read = get_mhu_handle();
    start_mcp_error_injection_listener(mhu_handle_read); // Need to get the mhu_handle
}

void start_mcp_error_injection_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    BUG_ASSERT_PARAM(icc_ctx != NULL, icc_ctx, 0);

    static uint8_t mcp_err_domain_injection_payload[SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE];
    static fpfw_icc_base_recv_req_t mcp_err_injection_recv_req = {0};

    mcp_err_injection_recv_req.recv_cmd_code = ICC_HM_ERROR_INJECTION_MCP;
    mcp_err_injection_recv_req.payload_buffer = mcp_err_domain_injection_payload;
    mcp_err_injection_recv_req.buffer_size = sizeof(mcp_err_domain_injection_payload);
    mcp_err_injection_recv_req.cb = mcp_error_injection_request_cb;
    mcp_err_injection_recv_req.cb_ctx = mcp_err_domain_injection_payload;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &mcp_err_injection_recv_req);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, icc_ctx);
}