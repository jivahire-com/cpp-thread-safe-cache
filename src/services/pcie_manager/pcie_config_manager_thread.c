//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements PCIe config manager thread.
 */

#include "FpFwAssert.h" // for FPFW_RUNTIME_ASSERT

#include <DfwkClient.h>   // for DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE
#include <ErrorHandler.h> // for FPFwErrorRaise
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <mscp_exp_rmss_memory_map.h>
#include <pcie_config_variable.h>
#include <pcie_manager_i.h>   // for rpss_req_completion_cb, rpss_service_t...
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <silibs_kng_soc.h>
#include <stdbool.h> // for true
#include <stdint.h>  // for uint8_t
#include <stdio.h>   // for fflush, printf, stdout
#include <tx_api.h>  // for TX_WAIT_FOREVER, ULONG, tx_queue_receive
#include <variable_services.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void icc_var_send_complete_cb(void* context, fpfw_status_t status);
static void icc_var_recv_complete_cb(void* context, size_t output_size_bytes, fpfw_status_t status);
static void set_hsp_variable(const guid_t* guid_ptr,
                             const uint16_t* variable_name_ptr,
                             size_t variable_name_size,
                             void* data_ptr,
                             size_t data_size);
/*-- Declarations (Statics and globals) --*/

static uint32_t rb_cb_ctx;
static uint32_t vab_cb_ctx;
static kng_hsp_mailbox_cmd_set_variable set_var_send_msg = {.header.cmd = HSP_MAILBOX_CMD_SET_VARIABLE_REQ,
                                                            .set_variable_address = SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE};
static kng_hsp_mailbox_msg set_var_recv_msg = {.header.cmd = HSP_MAILBOX_CMD_SET_VARIABLE_RSP};

static fpfw_icc_base_send_req_t set_var_send_req = {.payload_buffer = &set_var_send_msg,
                                                    .cb = icc_var_send_complete_cb,
                                                    .cb_ctx = &rb_cb_ctx,
                                                    .buffer_size = sizeof(kng_hsp_mailbox_msg)};

static fpfw_icc_base_recv_req_t set_var_recv_req = {
    .payload_buffer = &set_var_recv_msg,
    .buffer_size = sizeof(kng_hsp_mailbox_msg),
    .cb = icc_var_recv_complete_cb,
    .recv_cmd_code = HSP_MAILBOX_CMD_SET_VARIABLE_RSP,
    .cb_ctx = &rb_cb_ctx,
};

/* publish NV variable with RB aperture info */
static guid_t const rb_config_guid_die_0 = PCIE_RB_CONFIG_VAR_DIE_0_GUID;
static guid_t const vab_config_guid_die_0 = VAB_CONFIG_VAR_DIE_0_GUID;
static uint16_t const rb_config_varname_die_0[] = PCIE_RB_CONFIG_VAR_DIE_0_NAME;
static uint16_t const vab_config_varname_die_0[] = VAB_CONFIG_VAR_DIE_0_NAME;

static guid_t const rb_config_guid_die_1 = PCIE_RB_CONFIG_VAR_DIE_1_GUID;
static guid_t const vab_config_guid_die_1 = VAB_CONFIG_VAR_DIE_1_GUID;
static uint16_t const rb_config_varname_die_1[] = PCIE_RB_CONFIG_VAR_DIE_1_NAME;
static uint16_t const vab_config_varname_die_1[] = VAB_CONFIG_VAR_DIE_1_NAME;

static kingsgate_pcie_root_bridge_config* rb_config_var;
static kingsgate_pcie_vab_config* vab_config_var;
static const guid_t* rb_config_guid;
static const guid_t* vab_config_guid;
static const uint16_t* rb_config_varname;
static const uint16_t* vab_config_varname;

/*------------- Functions ----------------*/

/* Will be called when the SetVariable is completed from MSCP side */
static void icc_var_send_complete_cb(void* context, fpfw_status_t status)
{
    //! cb is raised when send is complete
    //! check status & do work
    if (context == &rb_cb_ctx)
    {
        printf("RB SetVariable send complete, status is %x\n", (int)status);
    }
    else if (context == &vab_cb_ctx)
    {
        printf("VAB SetVariable send complete, status is %x\n", (int)status);
    }
}

/* Will be called when the SetVariable is completed from HSP side */
static void icc_var_recv_complete_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    //! cb is raised when receive is complete
    //! check status & do work
    FPFW_UNUSED(output_size_bytes);

    if (context == &rb_cb_ctx)
    {
        printf("RB SetVariable response received, status is %x\n", (int)status);
        printf("received status from hsp: 0x%x\n", (int)set_var_recv_msg.rsp.status);
        set_var_send_req.cb_ctx = &vab_cb_ctx;
        set_var_recv_req.cb_ctx = &vab_cb_ctx;
        memset((void*)SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE, 0, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_SIZE);
        set_hsp_variable(vab_config_guid,
                         vab_config_varname,
                         sizeof(vab_config_varname_die_0),
                         (void*)vab_config_var,
                         sizeof(kingsgate_pcie_vab_config));
    }
    else if (context == &vab_cb_ctx)
    {
        printf("VAB SetVariable response received, status is %x\n", (int)status);
        printf("received status from hsp: 0x%x\n", (int)set_var_recv_msg.rsp.status);
        memset((void*)SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE, 0, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_SIZE);
    }
}

static void set_hsp_variable(const guid_t* guid_ptr, const uint16_t* variable_name_ptr, size_t variable_name_size, void* data_ptr, size_t data_size)
{
    uintptr_t target_addr = (uintptr_t)(SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE);

    struct hsp_mbox_set_variable set_var;
    set_var.variable_name_size = variable_name_size / sizeof(uint16_t);
    set_var.vendor_guid.guid.data1 = guid_ptr->guid1;
    set_var.vendor_guid.guid.data2 = guid_ptr->guid2;
    set_var.vendor_guid.guid.data3 = guid_ptr->guid3;
    set_var.vendor_guid.guid.data4[0] = guid_ptr->guid4[0];
    set_var.vendor_guid.guid.data4[1] = guid_ptr->guid4[1];
    set_var.vendor_guid.guid.data4[2] = guid_ptr->guid4[2];
    set_var.vendor_guid.guid.data4[3] = guid_ptr->guid4[3];
    set_var.vendor_guid.guid.data4[4] = guid_ptr->guid4[4];
    set_var.vendor_guid.guid.data4[5] = guid_ptr->guid4[5];
    set_var.vendor_guid.guid.data4[6] = guid_ptr->guid4[6];
    set_var.vendor_guid.guid.data4[7] = guid_ptr->guid4[7];
    set_var.attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;
    set_var.data_size = data_size;

    memcpy((void*)target_addr, (void*)&set_var, sizeof(set_var));
    target_addr += sizeof(set_var);

    memcpy((void*)target_addr, (void*)variable_name_ptr, variable_name_size);
    target_addr += variable_name_size;

    memcpy((void*)target_addr, data_ptr, data_size);
    target_addr += data_size;

    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)fpfw_init_get_handle("icc_hspmbx");

    fpfw_status_t recv_status = fpfw_icc_base_recv(icc_ctx, &set_var_recv_req);
    printf("SetVariable response: got status %x\n", (int)recv_status);

    fpfw_status_t send_status = fpfw_icc_base_send(icc_ctx, &set_var_send_req);
    printf("SetVariable request: got status %x\n", (int)send_status);

    FpFwAssert(target_addr <= SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_END);
}

void config_variable_service_thread_fn(ULONG thread_input)
{
    pcie_config_manager_context_t* ctx = (pcie_config_manager_context_t*)(thread_input);
    ULONG event;
    rb_config_var = ctx->rb_config_var;
    vab_config_var = ctx->vab_config_var;
    KNG_DIE_ID current_die_instance = (KNG_DIE_ID)idsw_get_die_id();
    static_assert(SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_SIZE > sizeof(struct hsp_mbox_set_variable) +
                                                                       sizeof(rb_config_varname_die_0) +
                                                                       sizeof(kingsgate_pcie_root_bridge_config),
                                                                       "ERROR: PCIE RB Variable payload size exceeded");

    static_assert(SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_SIZE > sizeof(struct hsp_mbox_set_variable) +
                                                                       sizeof(vab_config_varname_die_0) +
                                                                       sizeof(kingsgate_pcie_vab_config),
                                                                       "ERROR: PCIE VAB Variable payload size exceeded");

    int status = tx_event_flags_get(ctx->event_ptr, ctx->event_flags_mask, TX_AND, &event, TX_WAIT_FOREVER);
    if (status != TX_SUCCESS)
    {
        printf("%s: Failed to get event flags! TX_STATUS: %d\n", __func__, status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    printf("PCIe configs are set\n");
    tx_event_flags_delete(ctx->event_ptr);

    /* The BARs for accelerators should be added here, see definitions in silibs_kng_soc.h*/
    /* Note that there is no MMIOL for either RB*/
    if (current_die_instance == DIE_0)
    {
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].mmioh.base = D0_SDM_MMIOH_START;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].mmioh.limit = D0_SDM_MMIOH_END;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].bus.base = D0_SDM_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].bus.limit = D0_SDM_RCEC_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].flags.is_enabled = true;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].flags.is_integrated_endpoint = true;

        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].mmioh.base = D0_CDED_MMIOH_START;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].mmioh.limit = D0_CDED_MMIOH_END;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].bus.base = D0_CDED_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].bus.limit = D0_CDED_RCEC_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].flags.is_enabled = true;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].flags.is_integrated_endpoint = true;
    }
    else
    {
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].mmioh.base = D1_SDM_MMIOH_START;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].mmioh.limit = D1_SDM_MMIOH_END;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].bus.base = D1_SDM_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].bus.limit = D1_SDM_RCEC_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].flags.is_enabled = true;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].flags.is_integrated_endpoint = true;

        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].mmioh.base = D1_CDED_MMIOH_START;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].mmioh.limit = D1_CDED_MMIOH_END;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].bus.base = D1_CDED_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].bus.limit = D1_CDED_RCEC_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].flags.is_enabled = true;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].flags.is_integrated_endpoint = true;
    }

    /* Print RB configurations */
    for (uint8_t i = 0; i < PCIE_CONFIG_VAR_NUM_RB; i++)
    {
        /* Copy over the rb config */
        pcie_root_bridge_config* rb_config = &(rb_config_var->rootbridge_config[i]);

        if (rb_config->flags.is_enabled == false)
        {
            continue;
        }

        printf("RB config %d is enabled\n", i);

        printf("MMIO64 range 0x%08lx%08lx - 0x%08lx%08lx\n",
               (unsigned long)(rb_config->mmioh.base >> 32),
               (unsigned long)(rb_config->mmioh.base),
               (unsigned long)(rb_config->mmioh.limit >> 32),
               (unsigned long)(rb_config->mmioh.limit));

        printf("MMIO32 range 0x%lx - 0x%lx\n",
               (unsigned long)rb_config->mmiol.base,
               (unsigned long)rb_config->mmiol.limit);

        printf("bus range 0x%lx - 0x%lx\n", (unsigned long)rb_config->bus.base, (unsigned long)rb_config->bus.limit);
    }

    /* Print VAB configurations */
    for (uint8_t i = 0; i < PCIE_CONFIG_VAR_NUM_VAB; i++)
    {
        printf("VAB %d smmu_bypass %d\n", i, (int)(vab_config_var->perf_config[i].smmu_bypass));
    }

    if (current_die_instance == DIE_0)
    {
        rb_config_guid = &rb_config_guid_die_0;
        vab_config_guid = &vab_config_guid_die_0;
        rb_config_varname = rb_config_varname_die_0;
        vab_config_varname = vab_config_varname_die_0;
    }
    else
    {
        rb_config_guid = &rb_config_guid_die_1;
        vab_config_guid = &vab_config_guid_die_1;
        rb_config_varname = rb_config_varname_die_1;
        vab_config_varname = vab_config_varname_die_1;
    }

    set_hsp_variable(rb_config_guid,
                     rb_config_varname,
                     sizeof(rb_config_varname_die_0),
                     (void*)rb_config_var,
                     sizeof(kingsgate_pcie_root_bridge_config));
}