//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements a command line interface to interact with the pcie root port
 * management service on the SCP.
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h>
#include <FpFwCli.h>
#include <atu_api.h>
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <kng_error.h>
#include <kng_soc_constants.h>
#include <mscp_exp_rmss_memory_map.h>
#include <pcie_cli_i.h>
#include <pcie_dfwk.h>
#include <pcie_rp_common.h>
#include <pcie_ss_common.h>
#include <rc4sx16_pf0_type1_hdr_regs.h>
#include <silibs_status.h>
#include <stdint.h>
#include <stdlib.h>
#include <utils.h>
#include <variable_services.h>
#include <variable_services_mem.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static FPFW_CLI_STATUS dump_rpss_entity(int argc, const char** argv);
static FPFW_CLI_STATUS dump_rp_entity(int argc, const char** argv);
static FPFW_CLI_STATUS dump_rp_link_info(int argc, const char** argv);
static FPFW_CLI_STATUS dump_rp_dbi_cfg_hdr(int argc, const char** argv);
static FPFW_CLI_STATUS dump_pci_hsp_variables(int argc, const char** argv);
static void get_variable_cb(void* context, struct _variable_service_req_ctx* var_serv_ctx, uint8_t* data_start_ptr, size_t data_size);

/*-- Declarations (Statics and globals) --*/
/* clang-format off */
static FPFW_CLI_COMMAND pcie_cli_table[] = {
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_rpss_entity",
        .Routine = dump_rpss_entity,
        .Description = "RPSS entity info",
        .Usage = "dump_rpss_entity <rpss_idx(0-7)>"
    },
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_rp_entity",
        .Routine = dump_rp_entity,
        .Description = "RP entity info",
        .Usage = "dump_rp_entity <rpss_idx(0-7)> <rp_idx(0-3)>"
    },
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_rp_link_info",
        .Routine = dump_rp_link_info,
        .Description = "Show RP link info",
        .Usage = "dump_rp_link_info <rpss_idx(0-7)> <rp_idx(0-3)>"
    },
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_rp_dbi_cfg_hdr",
        .Routine = dump_rp_dbi_cfg_hdr,
        .Description = "Show RP Type1 config (DBI)",
        .Usage = "dump_rp_dbi_cfg_hdr <rpss_idx(0-7)> <rp_idx(0-3)>"
    },
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_pci_hsp_variables",
        .Routine = dump_pci_hsp_variables,
        .Description = "Dumps variables. var_idx values: 1 - DIE0 ROOT_BRIDGE, 2 - DIE0 VAB, 3 - DIE1 ROOT_BRIDGE, 4 - DIE1 VAB",
        .Usage = "dump_pci_hsp_variables <var_idx>"
    },
};
/* clang-format on */

static pciess_device_interface_t iface[PCIE_RPSS_PER_DIE] = {0};
static pcie_ss_entity_t* curr_ss_entity = NULL;

static uint32_t rb_cb_ctx;
static uint32_t vab_cb_ctx;

guid_t rb_config_guid_die_0 = PCIE_RB_CONFIG_VAR_DIE_0_GUID;
guid_t vab_config_guid_die_0 = VAB_CONFIG_VAR_DIE_0_GUID;
uint16_t rb_config_varname_die_0[] = PCIE_RB_CONFIG_VAR_DIE_0_NAME;
uint16_t vab_config_varname_die_0[] = VAB_CONFIG_VAR_DIE_0_NAME;

guid_t rb_config_guid_die_1 = PCIE_RB_CONFIG_VAR_DIE_1_GUID;
guid_t vab_config_guid_die_1 = VAB_CONFIG_VAR_DIE_1_GUID;
uint16_t rb_config_varname_die_1[] = PCIE_RB_CONFIG_VAR_DIE_1_NAME;
uint16_t vab_config_varname_die_1[] = VAB_CONFIG_VAR_DIE_1_NAME;

// Mem region to store the variable service payload
// Max size can hold either RB or VAB config
var_service_shared_mem_t var_svc_mem_ctx = {.payload_base = SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE,
                                            .max_payload_size = sizeof(variable_service_shared_mem_format_t) +
                                                                sizeof(rb_config_varname_die_0) +
                                                                sizeof(kingsgate_pcie_root_bridge_config)};

static var_service_req_ctx_t var_svc_req_ctx = {};

kingsgate_pcie_root_bridge_config* get_rb_config;
kingsgate_pcie_vab_config* get_vab_config;

/*------------- Functions ----------------*/

static PLACED_CODE void get_variable_cb(void* context, struct _variable_service_req_ctx* var_serv_ctx, uint8_t* data_start_ptr, size_t data_size)
{
    //! cb is raised when recieve is complete
    //! check status & do work
    FPFW_UNUSED(data_size);

    printf("PCIe GetVariable response received, status is %x\n", (int)var_serv_ctx->async_req_result);

    if (context == &rb_cb_ctx)
    {
        /* Print RB configurations */
        get_rb_config = (kingsgate_pcie_root_bridge_config*)data_start_ptr;
        for (uint8_t i = 0; i < PCIE_CONFIG_VAR_NUM_RB; i++)
        {
            /* Copy over the rb config */
            pcie_root_bridge_config* rb_config = &(get_rb_config->rootbridge_config[i]);

            if (rb_config->flags.is_enabled == false)
            {
                continue;
            }

            printf("RB config %d is enabled\n", i);

            printf("HEST AER UE Mask: 0x%08lx\n", (unsigned long)(rb_config->aer_ue_mask));

            printf("HEST AER UE Severity: 0x%08lx\n", (unsigned long)(rb_config->aer_ue_severity));

            printf("HEST AER CE Mask: 0x%08lx\n", (unsigned long)(rb_config->aer_ce_mask));

            printf("MMIO64 range 0x%08lx%08lx - 0x%08lx%08lx\n",
                   (unsigned long)(rb_config->mmioh.base >> 32),
                   (unsigned long)(rb_config->mmioh.base),
                   (unsigned long)(rb_config->mmioh.limit >> 32),
                   (unsigned long)(rb_config->mmioh.limit));

            printf("MMIO32 range 0x%lx - 0x%lx\n",
                   (unsigned long)rb_config->mmiol.base,
                   (unsigned long)rb_config->mmiol.limit);

            printf("bus range 0x%lx - 0x%lx\n",
                   (unsigned long)rb_config->bus.base,
                   (unsigned long)rb_config->bus.limit);
        }
        memset((void*)SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE, 0, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_SIZE);
    }
    else if (context == &vab_cb_ctx)
    {
        /* Print VAB configurations */
        get_vab_config = (kingsgate_pcie_vab_config*)data_start_ptr;
        for (uint8_t i = 0; i < PCIE_CONFIG_VAR_NUM_VAB; i++)
        {
            printf("VAB %d smmu_bypass %d\n", i, (int)(get_vab_config->perf_config[i].smmu_bypass));
            printf("VAB %d disable %d\n", i, (int)(get_vab_config->vab_config[i].vab_disable));
        }
        memset((void*)SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE, 0, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_SIZE);
    }
}

static PLACED_CODE FPFW_CLI_STATUS get_hsp_variable(guid_t* guid_ptr,
                                                    uint16_t* variable_name_ptr,
                                                    size_t variable_name_size,
                                                    size_t data_size,
                                                    uint32_t* cb_ctx)
{

    var_service_req_params_t req_params = {};
    req_params.variable_name_ptr = variable_name_ptr;
    req_params.variable_name_size = variable_name_size;
    memcpy(&req_params.vendor_namespace_guid, guid_ptr, sizeof(req_params.vendor_namespace_guid));
    req_params.data_size = data_size;
    req_params.data = NULL; // don't need this for get_var
    req_params.attributes.as_uint32 = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

    int status = variable_service_async_get_variable(&var_svc_req_ctx, &req_params, get_variable_cb, (void*)cb_ctx);

    if (status != KNG_SUCCESS)
    {
        FpFwCliPrint("[var_serv] Get Variable Async Request Failed\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS dump_pci_hsp_variables(int argc, const char** argv)
{
    if (argc != 2)
    {
        return CLI_ERROR;
    }

    switch (strtoul(argv[1], NULL, 0))
    {
    case (1):
        return get_hsp_variable(&rb_config_guid_die_0,
                                rb_config_varname_die_0,
                                sizeof(rb_config_varname_die_0),
                                sizeof(kingsgate_pcie_root_bridge_config),
                                &rb_cb_ctx);
        break;
    case (2):
        return get_hsp_variable(&vab_config_guid_die_0,
                                vab_config_varname_die_0,
                                sizeof(vab_config_varname_die_0),
                                sizeof(kingsgate_pcie_vab_config),
                                &vab_cb_ctx);
        break;
    case (3):
        return get_hsp_variable(&rb_config_guid_die_1,
                                rb_config_varname_die_1,
                                sizeof(rb_config_varname_die_1),
                                sizeof(kingsgate_pcie_root_bridge_config),
                                &rb_cb_ctx);
        break;
    case (4):
        return get_hsp_variable(&vab_config_guid_die_1,
                                vab_config_varname_die_1,
                                sizeof(vab_config_varname_die_1),
                                sizeof(kingsgate_pcie_vab_config),
                                &vab_cb_ctx);
        break;
    default:
        break;
    }

    return FPFW_STATUS_SUCCESS;
}
static PLACED_CODE FPFW_CLI_STATUS dump_rpss_entity(int argc, const char** argv)
{
    if (argc != 2)
    {
        return CLI_ERROR;
    }

    RPSS_INSTANCE rpss_idx = (RPSS_INSTANCE)(strtoul(argv[1], NULL, 0));
    if (rpss_idx >= NUM_RPSS)
    {
        return CLI_ERROR;
    }

    curr_ss_entity = NULL;
    pcie_sync_request_t req = {0};
    req.p_requested_data = (void*)(&curr_ss_entity);
    req.rpss_index = rpss_idx;
    req.req_type = GET_RPSS_ENTITY_REQUEST;
    req.header.RequestType = GET_RPSS_ENTITY_REQUEST;
    DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(&iface[rpss_idx % PCIE_RPSS_PER_DIE]), &req.header);

    if (req.status == SILIBS_SUCCESS && curr_ss_entity != NULL)
    {
        print_rpss_entity(curr_ss_entity);
    }
    else
    {
        FpFwCliPrint("Failed to get entity from the PCIe driver! (%d)\n", (int)req.status);
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS dump_rp_entity(int argc, const char** argv)
{
    if (argc != 3)
    {
        return CLI_ERROR;
    }

    RPSS_INSTANCE rpss_idx = (RPSS_INSTANCE)(strtoul(argv[1], NULL, 0));
    uint8_t rp_idx = (RPSS_INSTANCE)(strtoul(argv[2], NULL, 0));

    if (rpss_idx >= NUM_RPSS || rp_idx >= PCIESS_NUM_PORTS)
    {
        return CLI_ERROR;
    }

    curr_ss_entity = NULL;
    pcie_sync_request_t req = {0};
    req.p_requested_data = (void*)(&curr_ss_entity);
    req.rpss_index = rpss_idx;
    req.rp_index = rp_idx;
    req.req_type = GET_RPSS_ENTITY_REQUEST;
    req.header.RequestType = GET_RPSS_ENTITY_REQUEST;
    DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(&iface[rpss_idx % PCIE_RPSS_PER_DIE]), &req.header);

    if (req.status == SILIBS_SUCCESS && curr_ss_entity != NULL)
    {
        print_rp_entity(&(curr_ss_entity->rps[rp_idx]));
    }
    else
    {
        FpFwCliPrint("Failed to get entity from the PCIe driver! (%d)\n", (int)req.status);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS dump_rp_link_info(int argc, const char** argv)
{
    if (argc != 3)
    {
        return CLI_ERROR;
    }

    RPSS_INSTANCE rpss_idx = (RPSS_INSTANCE)(strtoul(argv[1], NULL, 0));
    uint8_t rp_idx = (RPSS_INSTANCE)(strtoul(argv[2], NULL, 0));

    if (rpss_idx >= NUM_RPSS || rp_idx >= PCIESS_NUM_PORTS)
    {
        return CLI_ERROR;
    }

    PCIE_LTSSM_STATE ltssm_state = 0;
    pcie_cli_req_op_t cli_op = GET_RP_LTSSM_STATE;
    pcie_link_state_t* current_link_state = {0};

    pcie_sync_request_t req = {0};
    req.rpss_index = rpss_idx;
    req.rp_index = rp_idx;
    req.req_type = CLI_REQUEST;
    req.header.RequestType = CLI_REQUEST;
    req.p_sent_data = (void*)(&cli_op);
    req.p_requested_data = (void*)(&ltssm_state);
    DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(&iface[rpss_idx % PCIE_RPSS_PER_DIE]), &req.header);
    if (req.status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("Failed to get ltssm state from the PCIe driver! (%d)\n", (int)req.status);
        return CLI_ERROR;
    }

    cli_op = GET_RP_LINK_STATE;
    req.p_requested_data = (void*)(&current_link_state);
    DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(&iface[rpss_idx]), &req.header);

    print_link_state(rpss_idx, rp_idx, ltssm_state, current_link_state);

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS dump_rp_dbi_cfg_hdr(int argc, const char** argv)
{
    if (argc != 3)
    {
        return CLI_ERROR;
    }

    RPSS_INSTANCE rpss_idx = (RPSS_INSTANCE)(strtoul(argv[1], NULL, 0));
    uint8_t rp_idx = (RPSS_INSTANCE)(strtoul(argv[2], NULL, 0));

    if (rpss_idx >= NUM_RPSS || rp_idx >= PCIESS_NUM_PORTS)
    {
        return CLI_ERROR;
    }

    rc4sx16_pf0_type1_hdr_reg* rp_t1_hdr = NULL;
    pcie_cli_req_op_t cli_op = GET_RP_DBI_CONFIG_HDR;

    /* Setup and send the DFWK request */
    pcie_sync_request_t req = {0};
    req.rpss_index = rpss_idx;
    req.rp_index = rp_idx;
    req.req_type = CLI_REQUEST;
    req.header.RequestType = CLI_REQUEST;
    req.p_sent_data = (void*)(&cli_op);
    req.p_requested_data = (void*)(&rp_t1_hdr);
    DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(&iface[rpss_idx % PCIE_RPSS_PER_DIE]), &req.header);

    if (req.status == SILIBS_SUCCESS)
    {
        print_type1_rp_header(rpss_idx, rp_idx, rp_t1_hdr);
    }
    else
    {
        FpFwCliPrint("Failed to get root port config from the PCIe driver! (%d)\n", (int)req.status);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

PLACED_CODE void pcie_cli_init(pciess_device_t* pcie_dev_handles)
{
    if (pcie_dev_handles == NULL)
    {
        return;
    }

    /* Initialize and open up PCIe interfaces so that we can issue requests to the PCIe driver later on */
    for (uint8_t i = 0; i < PCIE_RPSS_PER_DIE; i++)
    {
        pcie_dfwk_interface_init(&(pcie_dev_handles[i]), &(iface[i]));
        DfwkClientInterfaceOpen(&(iface[i].header));
    }

    KNG_DIE_ID current_die_instance = (KNG_DIE_ID)idsw_get_die_id();
    if (current_die_instance == DIE_0)
    {
        var_svc_mem_ctx.payload_base = SCP_EXP_SCP_VARIABLE_SERVICE_PAYLOAD_BASE;
    }
    else
    {
        var_svc_mem_ctx.payload_base = (uintptr_t)MSCP_ATU_AP_WINDOW_VAR_SVC_PCIE_PAYLOAD_BASE;
    }

    // Init RB variable service context
    variable_service_initialize_ctx(&var_svc_req_ctx, &var_svc_mem_ctx);

    FpFwCliRegisterTable(&pcie_cli_table[0], (sizeof(pcie_cli_table) / sizeof(FPFW_CLI_COMMAND)));
}
