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
#include <fpfw_status.h>
#include <health_monitor_icc.h>
#include <idsw_kng.h>
#include <kng_icc_shared.h>
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
#define SCF_RAM_ADDRESS   (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_RAM_ADDRESS)
#define RMSS_RAM0_ADDRESS (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS)
#define RMSS_RAM1_ADDRESS (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS)
#define DTC_RAM_ADDRESS   (MCP_TOP_MCP_DATA_RAM_ADDRESS)

/*-------- Function Prototypes -----------*/
void request_mcp_error_injection_setup(mcp_proc_err_type_t mcp_error_type, uint32_t trigger_addr);

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

static vptr_mscp_ras_and_init_ctrl_registers_reg mcp_ras_and_init_ctrl_registers_reg =
    (vptr_mscp_ras_and_init_ctrl_registers_reg)(MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS);

/*-------------- Functions ---------------*/
static uint32_t get_error_injection_address(ras_einj_info_t* einj_payload, atu_map_entry_t* atu_entry, atu_map_entry_t* atu_recover_entry)
{
    uint32_t err_addr = 0;

    switch (einj_payload->param_type.error_type)
    {
    case MCP_ERROR_TYPE_S_ARSM_CE:
    case MCP_ERROR_TYPE_S_ARSM_UE:
    case MCP_ERROR_TYPE_S_ARSM_OVERFLOW: {
        atu_entry_attr_t attributes = {ATU_BUS_ATTR_S};
        err_addr = get_arsm_attr_address(attributes.as_uint32, atu_entry, atu_recover_entry);
    }
    break;
    case MCP_ERROR_TYPE_NS_ARSM_CE:
    case MCP_ERROR_TYPE_NS_ARSM_UE:
    case MCP_ERROR_TYPE_NS_ARSM_OVERFLOW:
        // Non-secure ARSM is always mapped in MSCP with NS attribute
        err_addr = ((idsw_get_die_id() == DIE_0) ? MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR
                                                 : MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR);
        break;
    case MCP_ERROR_TYPE_RT_ARSM_CE:
    case MCP_ERROR_TYPE_RT_ARSM_UE:
    case MCP_ERROR_TYPE_RT_ARSM_OVERFLOW: {
        atu_entry_attr_t attributes = {ATU_BUS_ATTR_ROOT};
        err_addr = get_arsm_attr_address(attributes.as_uint32, atu_entry, atu_recover_entry);
    }
    break;
    case MCP_ERROR_TYPE_RL_ARSM_CE:
    case MCP_ERROR_TYPE_RL_ARSM_UE:
    case MCP_ERROR_TYPE_RL_ARSM_OVERFLOW: {
        atu_entry_attr_t attributes = {ATU_BUS_ATTR_REALM};
        err_addr = get_arsm_attr_address(attributes.as_uint32, atu_entry, atu_recover_entry);
    }
    break;

    default:
        FPFW_DBGPRINT_ERROR("Invalid MCP error type(%d)\n", einj_payload->param_type.error_type);
        break;
    }

    return err_addr;
}

static void trigger_arsm_fault(ras_einj_info_t* einj_payload, mscp_arsm_ram_type_t type, uint32_t err_mask)
{
    atu_map_entry_t arsm_atu_entry;
    atu_map_entry_t arsm_atu_recover_entry;
    trigger_shared_sram_arsm_fault(type, get_error_injection_address(einj_payload, &arsm_atu_entry, &arsm_atu_recover_entry), err_mask);
    BUG_ASSERT(atu_unmap(ATU_ID_MSCP, &arsm_atu_entry) == SILIBS_SUCCESS);
    if (arsm_atu_recover_entry.mscp_start_address != 0)
    {
        BUG_ASSERT(atu_map(ATU_ID_MSCP, &arsm_atu_recover_entry) == SILIBS_SUCCESS);
    }
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
            // TODO: (https://azurecsi.visualstudio.com/Dev/_workitems/edit/2662274/?view=edit)
            // Port SCP RAS handling to MCP (part 2) for error types with missing errctrl_reg
            switch (hm_err_injection_payload->error_injection_info.param_type.error_type)
            {
            case MCP_ERROR_TYPE_SCF_RAM_CE:
                request_mcp_error_injection_setup(MCP_ERROR_TYPE_SCF_RAM_CE, SCF_RAM_ADDRESS);
                break;

            case MCP_ERROR_TYPE_SCF_RAM_UE:
                request_mcp_error_injection_setup(MCP_ERROR_TYPE_SCF_RAM_UE, SCF_RAM_ADDRESS);
                break;

            case MCP_ERROR_TYPE_SCF_RAM_OVERFLOW:
                nvic_global_disable();
                // Request CE twice to trigger overflow
                request_mcp_error_injection_setup(MCP_ERROR_TYPE_SCF_RAM_OVERFLOW, SCF_RAM_ADDRESS);
                request_mcp_error_injection_setup(MCP_ERROR_TYPE_SCF_RAM_OVERFLOW, SCF_RAM_ADDRESS);
                nvic_global_enable();
                break;

            case MCP_ERROR_TYPE_RMSS_RAM0_CE:
                request_mcp_error_injection_setup(MCP_ERROR_TYPE_RMSS_RAM0_CE, RMSS_RAM0_ADDRESS);
                break;

            case MCP_ERROR_TYPE_RMSS_RAM0_UE:
                request_mcp_error_injection_setup(MCP_ERROR_TYPE_RMSS_RAM0_UE, RMSS_RAM0_ADDRESS);
                break;

            case MCP_ERROR_TYPE_RMSS_RAM0_OVERFLOW:
                nvic_global_disable();
                // Request CE twice to trigger overflow
                request_mcp_error_injection_setup(MCP_ERROR_TYPE_RMSS_RAM0_OVERFLOW, RMSS_RAM0_ADDRESS);
                request_mcp_error_injection_setup(MCP_ERROR_TYPE_RMSS_RAM0_OVERFLOW, RMSS_RAM0_ADDRESS);
                nvic_global_enable();
                break;

            case MCP_ERROR_TYPE_RMSS_RAM1_CE:
                request_mcp_error_injection_setup(MCP_ERROR_TYPE_RMSS_RAM1_CE, RMSS_RAM1_ADDRESS);
                break;

            case MCP_ERROR_TYPE_RMSS_RAM1_UE:
                request_mcp_error_injection_setup(MCP_ERROR_TYPE_RMSS_RAM1_UE, RMSS_RAM1_ADDRESS);
                break;

            case MCP_ERROR_TYPE_RMSS_RAM1_OVERFLOW:
                nvic_global_disable();
                // Request CE twice to trigger overflow
                request_mcp_error_injection_setup(MCP_ERROR_TYPE_RMSS_RAM1_OVERFLOW, RMSS_RAM1_ADDRESS);
                request_mcp_error_injection_setup(MCP_ERROR_TYPE_RMSS_RAM1_OVERFLOW, RMSS_RAM1_ADDRESS);
                nvic_global_enable();
                break;

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
                trigger_arsm_fault(&hm_err_injection_payload->error_injection_info,
                                   MSCP_S_ARSM_RAM,
                                   SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
                break;
            case MCP_ERROR_TYPE_S_ARSM_UE:
                if (!IS_PLATFORM_FPGA())
                {
                    trigger_arsm_fault(&hm_err_injection_payload->error_injection_info,
                                       MSCP_S_ARSM_RAM,
                                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
                }
                else
                {
                    FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_S_ARSM_UE is not supported on FPGA.\n");
                }
                break;
            case MCP_ERROR_TYPE_S_ARSM_OVERFLOW:
                if (!IS_PLATFORM_FPGA())
                {
                    trigger_arsm_fault(&hm_err_injection_payload->error_injection_info,
                                       MSCP_S_ARSM_RAM,
                                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
                }
                else
                {
                    FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_S_ARSM_OVERFLOW is not supported on FPGA.\n");
                }
                break;
            case MCP_ERROR_TYPE_NS_ARSM_CE:
                trigger_shared_sram_arsm_fault(
                    MSCP_NS_ARSM_RAM,
                    get_error_injection_address(&hm_err_injection_payload->error_injection_info, NULL, NULL),
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
                break;
            case MCP_ERROR_TYPE_NS_ARSM_UE:
                if (!IS_PLATFORM_FPGA())
                {
                    trigger_shared_sram_arsm_fault(
                        MSCP_NS_ARSM_RAM,
                        get_error_injection_address(&hm_err_injection_payload->error_injection_info, NULL, NULL),
                        SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
                }
                else
                {
                    FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_NS_ARSM_UE is not supported on FPGA.\n");
                }
                break;
            case MCP_ERROR_TYPE_NS_ARSM_OVERFLOW:
                if (!IS_PLATFORM_FPGA())
                {
                    trigger_shared_sram_arsm_fault(
                        MSCP_NS_ARSM_RAM,
                        get_error_injection_address(&hm_err_injection_payload->error_injection_info, NULL, NULL),
                        SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
                }
                else
                {
                    FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_NS_ARSM_OVERFLOW is not supported on FPGA.\n");
                }
                break;
            case MCP_ERROR_TYPE_RT_ARSM_CE:
                trigger_arsm_fault(&hm_err_injection_payload->error_injection_info,
                                   MSCP_RT_ARSM_RAM,
                                   SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
                break;
            case MCP_ERROR_TYPE_RT_ARSM_UE:
                if (!IS_PLATFORM_FPGA())
                {
                    trigger_arsm_fault(&hm_err_injection_payload->error_injection_info,
                                       MSCP_RT_ARSM_RAM,
                                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
                }
                else
                {
                    FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_RT_ARSM_UE is not supported on FPGA.\n");
                }
                break;
            case MCP_ERROR_TYPE_RT_ARSM_OVERFLOW:
                if (!IS_PLATFORM_FPGA())
                {
                    trigger_arsm_fault(&hm_err_injection_payload->error_injection_info,
                                       MSCP_RT_ARSM_RAM,
                                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
                }
                else
                {
                    FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_RT_ARSM_OVERFLOW is not supported on FPGA.\n");
                }
                break;
            case MCP_ERROR_TYPE_RL_ARSM_CE:
                trigger_arsm_fault(&hm_err_injection_payload->error_injection_info,
                                   MSCP_RL_ARSM_RAM,
                                   SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
                break;
            case MCP_ERROR_TYPE_RL_ARSM_UE:
                if (!IS_PLATFORM_FPGA())
                {
                    trigger_arsm_fault(&hm_err_injection_payload->error_injection_info,
                                       MSCP_RL_ARSM_RAM,
                                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
                }
                else
                {
                    FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_RL_ARSM_UE is not supported on FPGA.\n");
                }
                break;
            case MCP_ERROR_TYPE_RL_ARSM_OVERFLOW:
                if (!IS_PLATFORM_FPGA())
                {
                    trigger_arsm_fault(&hm_err_injection_payload->error_injection_info,
                                       MSCP_RL_ARSM_RAM,
                                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
                }
                else
                {
                    FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_RL_ARSM_OVERFLOW is not supported on FPGA.\n");
                }
                break;
            case MCP_ERROR_TYPE_RSM_RAM_CE:
                atu_map_entry_t rsm_atu_entry;
                uint32_t read_addr = map_rsm_address(&rsm_atu_entry);
                trigger_shared_sram_rsm_fault(MSCP_S_RSM_RAM, read_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
                unmap_rsm_address(&rsm_atu_entry);
                break;

            case MCP_ERROR_TYPE_RSM_RAM_UE:
                if (!IS_PLATFORM_FPGA())
                {
                    atu_map_entry_t rsm_atu_entry;
                    uint32_t read_addr = map_rsm_address(&rsm_atu_entry);
                    trigger_shared_sram_rsm_fault(MSCP_S_RSM_RAM, read_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
                    unmap_rsm_address(&rsm_atu_entry);
                }
                else
                {
                    FPFW_DBGPRINT_WARNING("MCP_ERROR_TYPE_RSM_RAM_UE is not supported on FPGA.\n");
                }
                break;
            case MCP_ERROR_TYPE_M7_LOCKUP:
                trigger_lockup();
                break;

            case MCP_ERROR_TYPE_ATU_ERR:
                trigger_atu_error();
                break;

            default:
                FPFW_DBGPRINT_ERROR("Invalid/Unsupported MCP error type(%d)\n",
                                    hm_err_injection_payload->error_injection_info.param_type.error_type);
                break;
            }
        }
    }

    start_mcp_error_injection_listener(get_mhu_handle()); // Need to get the mhu_handle
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

void mcp_error_injection_setup_cb(void* context, fpfw_status_t status)
{
    if (status == FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        uint32_t trigger_addr = (uint32_t)context;
        FPFW_DBGPRINT_INFO("Access %p to trigger error\n", (void*)trigger_addr);
        inject_err_by_access(trigger_addr);
    }
}

void request_mcp_error_injection_setup(mcp_proc_err_type_t mcp_error_type, uint32_t trigger_addr)
{
    static uint8_t mcp_error_injection_setup_req_payload[32] = {0};
    icc_mhu_mscp_err_injection_setup_packet_t* err_setup_msg =
        (icc_mhu_mscp_err_injection_setup_packet_t*)mcp_error_injection_setup_req_payload;

    err_setup_msg->header.msg_header.command = ICC_HM_ERROR_INJECTION_SETUP_REQ;
    err_setup_msg->header.msg_header.payload_size = sizeof(uint32_t);
    err_setup_msg->mcp_error_type = mcp_error_type;

    static fpfw_icc_base_send_req_t mcp_send_params;
    mcp_send_params.payload_buffer = mcp_error_injection_setup_req_payload;
    mcp_send_params.buffer_size = sizeof(mcp_error_injection_setup_req_payload);
    mcp_send_params.cb = mcp_error_injection_setup_cb;
    mcp_send_params.cb_ctx = (void*)trigger_addr;

    fpfw_icc_base_ctx_t* icc_ctx = get_mhu_handle();
    BUG_ASSERT_PARAM(icc_ctx != NULL, icc_ctx, 0);

    fpfw_status_t icc_status = fpfw_icc_base_send(icc_ctx, &mcp_send_params);
    if (icc_status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("Error Injection failed (%d)\n", icc_status);
    }
}