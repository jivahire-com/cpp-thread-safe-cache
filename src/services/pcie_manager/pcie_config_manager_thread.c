//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements PCIe config manager thread.
 */

#include <DbgPrint.h>
#include <DfwkClient.h>     // for DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE
#include <ErrorHandler.h>   // for FPFwErrorRaise
#include <FpFwAssert.h>     // for FPFW_RUNTIME_ASSERT
#include <accelerator_ip.h> // For Accel isolation
#include <atu_api.h>
#include <bug_check.h>
#include <idsw_kng.h>
#include <kng_error.h>
#include <mscp_exp_rmss_memory_map.h>
#include <pcie_config_variable.h>
#include <pcie_manager_i.h>   // for rpss_req_completion_cb, rpss_service_t...
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <silibs_kng_soc.h>
#include <stdbool.h> // for true
#include <stdint.h>  // for uint8_t
#include <tx_api.h>  // for TX_WAIT_FOREVER, ULONG, tx_queue_receive
#include <variable_services.h>
#include <variable_services_mem.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void set_var_complete_cb(void* context, var_service_req_ctx_t* var_serv_ctx, uint8_t* data_start_ptr, size_t data_size);
/*-- Declarations (Statics and globals) --*/

static uint32_t rb_cb_ctx;
static uint32_t vab_cb_ctx;

/* publish NV variable with RB aperture info */
static guid_t const rb_config_guid_die_0 = PCIE_RB_CONFIG_VAR_DIE_0_GUID;
static guid_t const vab_config_guid_die_0 = VAB_CONFIG_VAR_DIE_0_GUID;
static uint16_t rb_config_varname_die_0[] = PCIE_RB_CONFIG_VAR_DIE_0_NAME;
static uint16_t vab_config_varname_die_0[] = VAB_CONFIG_VAR_DIE_0_NAME;

static guid_t const rb_config_guid_die_1 = PCIE_RB_CONFIG_VAR_DIE_1_GUID;
static guid_t const vab_config_guid_die_1 = VAB_CONFIG_VAR_DIE_1_GUID;
static uint16_t rb_config_varname_die_1[] = PCIE_RB_CONFIG_VAR_DIE_1_NAME;
static uint16_t vab_config_varname_die_1[] = VAB_CONFIG_VAR_DIE_1_NAME;

static kingsgate_pcie_root_bridge_config* rb_config_var;
static kingsgate_pcie_vab_config* vab_config_var;
static const guid_t* rb_config_guid;
static const guid_t* vab_config_guid;
static uint16_t* rb_config_varname;
static uint16_t* vab_config_varname;

var_service_shared_mem_t rb_set_var_mem_ctx = {.payload_base = SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE,
                                               .max_payload_size = sizeof(variable_service_shared_mem_format_t) +
                                                                   sizeof(rb_config_varname_die_0) +
                                                                   sizeof(kingsgate_pcie_root_bridge_config)};

var_service_shared_mem_t vab_set_var_mem_ctx = {.payload_base = SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE,
                                                .max_payload_size = sizeof(variable_service_shared_mem_format_t) +
                                                                    sizeof(vab_config_varname_die_0) +
                                                                    sizeof(kingsgate_pcie_vab_config)};

static var_service_req_ctx_t rb_set_var_ctx = {};
static var_service_req_ctx_t vab_set_var_ctx = {};

/*------------- Functions ----------------*/

/* Will be called when the SetVariable is completed from HSP side */
void set_var_complete_cb(void* context, var_service_req_ctx_t* var_serv_ctx, uint8_t* data_start_ptr, size_t data_size)
{
    //! cb is raised when receive is complete
    //! check status & do work
    FPFW_UNUSED(data_size);
    FPFW_UNUSED(data_start_ptr);

    int status;

    if (context == &rb_cb_ctx)
    {
        FPFW_DBGPRINT_INFO("[PCIe Config] RB set_variable response received! Status: %x\n", (int)var_serv_ctx->async_req_result);
        // unlock context so we can send another for VAB
        // variable_service_unlock_get_var_ctx(var_serv_ctx);
        var_service_req_params_t req_params = {};
        req_params.variable_name_ptr = (uint16_t*)vab_config_varname;
        req_params.variable_name_size = sizeof(vab_config_varname_die_0);
        memcpy(&req_params.vendor_namespace_guid, vab_config_guid, sizeof(req_params.vendor_namespace_guid));
        req_params.data_size = sizeof(kingsgate_pcie_vab_config);
        req_params.data = (uint8_t*)vab_config_var;
        req_params.attributes.as_uint32 = EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;
        status = variable_service_async_set_variable(&vab_set_var_ctx, &req_params, set_var_complete_cb, (void*)&vab_cb_ctx);
        BUG_ASSERT(status == KNG_SUCCESS);
    }
    else if (context == &vab_cb_ctx)
    {
        FPFW_DBGPRINT_INFO("[PCIe Config] VAB set_variable response received! Status: %x\n",
                           (int)var_serv_ctx->async_req_result);
    }
}

void config_variable_service_thread_fn(ULONG thread_input)
{
    pcie_config_manager_context_t* ctx = (pcie_config_manager_context_t*)(thread_input);
    ULONG event;
    rb_config_var = ctx->rb_config_var;
    vab_config_var = ctx->vab_config_var;
    KNG_DIE_ID current_die_instance = (KNG_DIE_ID)idsw_get_die_id();

    bool sdm_enable = !(accel_is_isolation_enabled(ACCEL_ID_SDM)); // If SDM isolation enabled, sdm_enable == false
    bool cded_enable = !(accel_is_isolation_enabled(ACCEL_ID_CDED));

    FPFW_DBGPRINT_INFO("Die[%d] \t SDM: %d \t CDED %d \n", (int)current_die_instance, (int)sdm_enable, (int)cded_enable);
    static_assert(SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_SIZE > sizeof(variable_service_shared_mem_format_t) +
                                                                       sizeof(rb_config_varname_die_0) +
                                                                       sizeof(kingsgate_pcie_root_bridge_config),
                  "ERROR: PCIE RB Variable payload size exceeded");

    static_assert(SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_SIZE > sizeof(variable_service_shared_mem_format_t) +
                                                                       sizeof(vab_config_varname_die_0) +
                                                                       sizeof(kingsgate_pcie_vab_config),
                  "ERROR: PCIE VAB Variable payload size exceeded");
    int status = tx_event_flags_get(ctx->event_ptr, ctx->event_flags_mask, TX_AND, &event, TX_WAIT_FOREVER);
    if (status != TX_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("[PCIe Config] Failed to get event flags! TX_STATUS: %d\n", status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    FPFW_DBGPRINT_INFO("[PCIe Config] PCIe configs are set\n");
    tx_event_flags_delete(ctx->event_ptr);

    /* The BARs for accelerators should be added here, see definitions in silibs_kng_soc.h*/
    /* Note that there is no MMIOL for either RB*/
    if (current_die_instance == DIE_0)
    {
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].mmiol.base = UINT32_MAX;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].mmiol.limit = 0x0;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].mmioh.base = D0_SDM_MMIOH_START;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].mmioh.limit = D0_SDM_MMIOH_END;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].bus.base = D0_SDM_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].bus.limit = D0_SDM_RCEC_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].flags.is_enabled = sdm_enable;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].flags.is_integrated_endpoint = true;

        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].mmiol.base = UINT32_MAX;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].mmiol.limit = 0x0;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].mmioh.base = D0_CDED_MMIOH_START;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].mmioh.limit = D0_CDED_MMIOH_END;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].bus.base = D0_CDED_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].bus.limit = D0_CDED_RCEC_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].flags.is_enabled = cded_enable;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].flags.is_integrated_endpoint = true;
    }
    else
    {
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].mmiol.base = UINT32_MAX;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].mmiol.limit = 0x0;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].mmioh.base = D1_SDM_MMIOH_START;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].mmioh.limit = D1_SDM_MMIOH_END;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].bus.base = D1_SDM_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].bus.base = D1_SDM_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].bus.limit = D1_SDM_RCEC_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].flags.is_enabled = sdm_enable;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS].flags.is_integrated_endpoint = true;

        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].mmiol.base = UINT32_MAX;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].mmiol.limit = 0x0;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].mmioh.base = D1_CDED_MMIOH_START;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].mmioh.limit = D1_CDED_MMIOH_END;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].bus.base = D1_CDED_RCIEP_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].bus.limit = D1_CDED_RCEC_BUS;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].flags.is_enabled = cded_enable;
        rb_config_var->rootbridge_config[PCIE_RPSS_PER_DIE * PCIESS_NUM_PORTS + 1].flags.is_integrated_endpoint = true;
    }

    /* Print RB configurations */
    for (uint8_t i = 0; i < PCIE_CONFIG_VAR_NUM_RB; i++)
    {
        /* Copy over the rb config */
        pcie_root_bridge_config* rb_config = &(rb_config_var->rootbridge_config[i]);

        if (rb_config->flags.is_enabled == false)
        {
            FPFW_DBGPRINT_INFO("RB[%d]: Disabled\n", i);
            continue;
        }

        FPFW_DBGPRINT_INFO("RB[%d]: Enabled\n", i);

        FPFW_DBGPRINT_INFO("RB[%d]: MMIO64 range 0x%08lx%08lx - 0x%08lx%08lx\n",
                           i,
                           (unsigned long)(rb_config->mmioh.base >> 32),
                           (unsigned long)(rb_config->mmioh.base),
                           (unsigned long)(rb_config->mmioh.limit >> 32),
                           (unsigned long)(rb_config->mmioh.limit));

        FPFW_DBGPRINT_INFO("RB[%d]: MMIO32 range 0x%lx - 0x%lx\n",
                           i,
                           (unsigned long)rb_config->mmiol.base,
                           (unsigned long)rb_config->mmiol.limit);

        FPFW_DBGPRINT_INFO("RB[%d]: Bus range 0x%lx - 0x%lx\n",
                           i,
                           (unsigned long)rb_config->bus.base,
                           (unsigned long)rb_config->bus.limit);
    }

    /* Print VAB configurations */
    for (uint8_t i = 0; i < PCIE_CONFIG_VAR_NUM_VAB; i++)
    {
        FPFW_DBGPRINT_INFO("VAB[%d]: smmu_bypass: %d\n", i, (int)(vab_config_var->perf_config[i].smmu_bypass));
        FPFW_DBGPRINT_INFO("VAB[%d]: disable: %d\n", i, (int)(vab_config_var->vab_config[i].vab_disable));
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
        // By default the payload is in the MSCP EXP RAM but Die 1 HSP cannot access it
        // So Die 1 payload is in DDR
        rb_set_var_mem_ctx.payload_base = (uintptr_t)MSCP_ATU_AP_WINDOW_VAR_SVC_PCIE_PAYLOAD_BASE;
    }

    // Init RB variable service context
    variable_service_initialize_ctx(&rb_set_var_ctx, &rb_set_var_mem_ctx);

    // Init VAB variable service context
    vab_set_var_mem_ctx.payload_base = rb_set_var_mem_ctx.payload_base + rb_set_var_mem_ctx.max_payload_size;
    variable_service_initialize_ctx(&vab_set_var_ctx, &vab_set_var_mem_ctx);

    var_service_req_params_t req_params = {};
    req_params.variable_name_ptr = (uint16_t*)rb_config_varname;
    req_params.variable_name_size = sizeof(rb_config_varname_die_0);
    memcpy(&req_params.vendor_namespace_guid, rb_config_guid, sizeof(req_params.vendor_namespace_guid));
    req_params.data_size = sizeof(kingsgate_pcie_root_bridge_config);
    req_params.data = (uint8_t*)rb_config_var;
    req_params.attributes.as_uint32 = EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

    status = variable_service_async_set_variable(&rb_set_var_ctx, &req_params, set_var_complete_cb, (void*)&rb_cb_ctx);

    BUG_ASSERT(status == KNG_SUCCESS);
}
