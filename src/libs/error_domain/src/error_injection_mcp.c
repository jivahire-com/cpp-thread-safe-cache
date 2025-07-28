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
#include <atu_api.h>
#include <bug_check.h>
#include <cper.h>
#include <error_domain_i.h>
#include <health_monitor_icc.h>
#include <idsw_kng.h>
#include <mcp_exp_top_regs.h>
#include <mscp_error_domain.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_ras_and_init_ctrl_registers_regs.h>
#include <nvic.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <mcp_top_regs.h>
#include <shared_sram_ecc_ras_registers_regs.h>

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

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static fpfw_icc_base_ctx_t* mhu_handle_read = NULL;
static vptr_mscp_ras_and_init_ctrl_registers_reg mcp_ras_and_init_ctrl_registers_reg =
    (vptr_mscp_ras_and_init_ctrl_registers_reg)(MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS);

/*-------------- Functions ---------------*/

static void mcp_error_injection_request_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    if (status == FPFW_STATUS_SUCCESS)
    {
        hm_mhu_error_injection_payload_t* hm_err_injection_payload = (hm_mhu_error_injection_payload_t*)context;

        // ARSM RAM error injection address
        uint32_t arsm_addr = ((idsw_get_die_id() == DIE_0) ? MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR
                                                           : MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR);

        arsm_addr =
            (hm_err_injection_payload->error_injection_info.param_type.address_64 == 0)
                ? arsm_addr + ARSM_RAM_DEFAULT_OFFSET
                : arsm_addr + (uint32_t)(uintptr_t)hm_err_injection_payload->error_injection_info.param_type.address_64;

        if (hm_err_injection_payload != NULL && hm_err_injection_payload->header.msg_header.command == ICC_HM_ERROR_INJECTION_MCP)
        {
            // TODO: (https://azurecsi.visualstudio.com/Dev/_workitems/edit/2662274/?view=edit)
            // Port SCP RAS handling to MCP (part 2) for error types with missing errctrl_reg
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
            case MCP_ERROR_TYPE_DATA_CACHE_CE:
                dcache_ce_isr();
                break;
            case MCP_ERROR_TYPE_DATA_CACHE_UE:
                dcache_ue_isr();
                break;
            case MCP_ERROR_TYPE_DATA_CACHE_TAG_CE:
                dcache_tag_ce_isr();
                break;
            case MCP_ERROR_TYPE_DATA_CACHE_TAG_UE:
                dcache_tag_ue_isr();
                break;
            case MCP_ERROR_TYPE_INSTRUCTION_CACHE_CE:
                icache_ce_isr();
                break;
            case MCP_ERROR_TYPE_INSTRUCTION_CACHE_UE:
                icache_ue_isr();
                break;
            case MCP_ERROR_TYPE_INSTRUCTION_CACHE_TAG_CE:
                icache_tag_ce_isr();
                break;
            case MCP_ERROR_TYPE_INSTRUCTION_CACHE_TAG_UE:
                icache_tag_ue_isr();
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

            case MCP_ERROR_TYPE_S_ARSM_CE:
                trigger_shared_sram_arsm_fault(MSCP_RT_ARSM_RAM, arsm_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
                break;
            case MCP_ERROR_TYPE_S_ARSM_UE:
                FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_S_ARSM_UE is not supported in this build.\n");
                // trigger_shared_sram_arsm_fault(MSCP_RT_ARSM_RAM,
                //                           arsm_addr,
                //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
                break;
            case MCP_ERROR_TYPE_S_ARSM_OVERFLOW:
                FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_S_ARSM_OVERFLOW is not supported in this build.\n");
                // trigger_shared_sram_arsm_fault(MSCP_RT_ARSM_RAM,
                //                           arsm_addr,
                //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
                break;
            case MCP_ERROR_TYPE_NS_ARSM_CE:
                trigger_shared_sram_arsm_fault(MSCP_NS_ARSM_RAM, arsm_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
                break;
            case MCP_ERROR_TYPE_NS_ARSM_UE:
                FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_NS_ARSM_UE is not supported in this build.\n");
                // trigger_shared_sram_arsm_fault(MSCP_NS_ARSM_RAM,
                //                           arsm_addr,
                //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
                break;
            case MCP_ERROR_TYPE_NS_ARSM_OVERFLOW:
                FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_NS_ARSM_OVERFLOW is not supported in this build.\n");
                // trigger_shared_sram_arsm_fault(MSCP_NS_ARSM_RAM,
                //                           arsm_addr,
                //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
                break;
            case MCP_ERROR_TYPE_RT_ARSM_CE:
                trigger_shared_sram_arsm_fault(MSCP_RT_ARSM_RAM, arsm_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
                break;
            case MCP_ERROR_TYPE_RT_ARSM_UE:
                FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_RT_ARSM_UE is not supported in this build.\n");
                // trigger_shared_sram_arsm_fault(MSCP_RT_ARSM_RAM,
                //                           arsm_addr,
                //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
                break;
            case MCP_ERROR_TYPE_RT_ARSM_OVERFLOW:
                FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_RT_ARSM_OVERFLOW is not supported in this build.\n");
                // trigger_shared_sram_arsm_fault(MSCP_RT_ARSM_RAM,
                //                           arsm_addr,
                //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
                break;
            case MCP_ERROR_TYPE_RL_ARSM_CE:
                trigger_shared_sram_arsm_fault(MSCP_RL_ARSM_RAM, arsm_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
                break;
            case MCP_ERROR_TYPE_RL_ARSM_UE:
                FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_RL_ARSM_UE is not supported in this build.\n");
                // trigger_shared_sram_arsm_fault(MSCP_RL_ARSM_RAM,
                //                           arsm_addr,
                //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
                break;
            case MCP_ERROR_TYPE_RL_ARSM_OVERFLOW:
                FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_RL_ARSM_OVERFLOW is not supported in this build.\n");
                // trigger_shared_sram_arsm_fault(MSCP_RL_ARSM_RAM,
                //                           arsm_addr,
                //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
                break;
            case MCP_ERROR_TYPE_RSM_RAM_CE:
                atu_map_entry_t rsm_atu_entry;
                uint32_t read_addr = map_rsm_address(&rsm_atu_entry);
                trigger_shared_sram_rsm_fault(MSCP_S_RSM_RAM, read_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
                unmap_rsm_address(&rsm_atu_entry);
                break;

            case MCP_ERROR_TYPE_RSM_RAM_UE:
                FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_RSM_RAM_UE is not supported in this build.\n");
                // atu_map_entry_t rsm_atu_entry;
                // uint32_t read_addr = map_rsm_address(&rsm_atu_entry);
                // trigger_shared_sram_rsm_fault(MSCP_S_RSM_RAM, read_addr,
                // SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK); unmap_rsm_address(&rsm_atu_entry);
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