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
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <hsp_firmware_headers.h>
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

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static FPFW_CLI_STATUS dump_rpss_entity(int argc, const char** argv);
static FPFW_CLI_STATUS dump_rp_entity(int argc, const char** argv);
static FPFW_CLI_STATUS dump_rp_link_info(int argc, const char** argv);
static FPFW_CLI_STATUS dump_rp_dbi_cfg_hdr(int argc, const char** argv);
static void icc_get_var_recv_complete_cb(void* context, size_t output_size_bytes, fpfw_status_t status);
static void icc_get_var_send_complete_cb(void* context, fpfw_status_t status);
static FPFW_CLI_STATUS dump_pci_hsp_variables(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/
/* clang-format off */
static FPFW_CLI_COMMAND pcie_cli_table[] = {
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_rpss_entity",
        .Routine = dump_rpss_entity,
        .Description = "Shows rpss entity characteristics",
        .Usage = "pcie dump_rpss_entity <rpss_number between 0-7>"
    },
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_rp_entity",
        .Routine = dump_rp_entity,
        .Description = "Shows rp entity characteristics",
        .Usage = "pcie dump_rp_entity <rpss_number between 0-7> <rp_number between 0-3>"
    },
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_rp_link_info",
        .Routine = dump_rp_link_info,
        .Description = "Shows link information for an enabled root port",
        .Usage = "pcie dump_rp_link_info <rpss_number between 0-7> <rp_number between 0-3>"
    },
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_rp_dbi_cfg_hdr",
        .Routine = dump_rp_dbi_cfg_hdr,
        .Description = "Shows the root port type 1 configuration header (over DBI) for an enabled root port",
        .Usage = "pcie dump_rp_dbi_cfg_hdr <rpss_number between 0-7> <rp_number between 0-3>"
    },
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_pci_hsp_variables",
        .Routine = dump_pci_hsp_variables,
        .Description = "Dumps the system variables used for PCIe. var_index values: 1 - DIE0 ROOT_BRIDGE, 2 - DIE0 VAB, 3 - DIE1 ROOT_BRIDGE, 4 - DIE1 VAB",
        .Usage = "dump_pci_hsp_variables <var_index>"
    },
};
/* clang-format on */

static pciess_device_interface_t iface[PCIE_RPSS_PER_DIE] = {0};
static pcie_ss_entity_t* curr_ss_entity = NULL;

static uint32_t rb_cb_ctx;
static uint32_t vab_cb_ctx;
static kng_hsp_mailbox_cmd_get_variable get_send_msg = {.header.cmd = HSP_MAILBOX_CMD_GET_VARIABLE_REQ,
                                                        .get_variable_address = SCP_EXP_SCP_VARIABLE_SERVICE_PAYLOAD_BASE};

static kng_hsp_mailbox_msg get_recv_msg = {.header.cmd = HSP_MAILBOX_CMD_GET_VARIABLE_RSP};

static fpfw_icc_base_send_req_t get_var_send_req = {.payload_buffer = &get_send_msg,
                                                    .cb = icc_get_var_send_complete_cb,
                                                    .cb_ctx = NULL,
                                                    .buffer_size = sizeof(kng_hsp_mailbox_msg)};

static fpfw_icc_base_recv_req_t get_var_recv_req = {
    .payload_buffer = &get_recv_msg,
    .buffer_size = sizeof(kng_hsp_mailbox_msg),
    .cb = icc_get_var_recv_complete_cb,
    .recv_cmd_code = HSP_MAILBOX_CMD_GET_VARIABLE_RSP,
    .cb_ctx = NULL,
};

kingsgate_pcie_root_bridge_config* get_rb_config;
kingsgate_pcie_vab_config* get_vab_config;

/*------------- Functions ----------------*/

static void icc_get_var_send_complete_cb(void* context, fpfw_status_t status)
{
    //! cb is raised when send is complete
    //! check status & do work
    FPFW_UNUSED(context);
    printf("icc getvar send complete, status is %x\n", (int)status);
}

static void icc_get_var_recv_complete_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    //! cb is raised when recieve is complete
    //! check status & do work
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);
    printf("icc get_var rcv complete, status is 0x%x\n", (int)status);
    printf("received status = 0x%x\n", (int)get_recv_msg.rsp.status);
    if (status != 0 || get_recv_msg.rsp.status != 0)
    {
        return;
    }

    if (context == &rb_cb_ctx)
    {
        /* Print RB configurations */
        for (uint8_t i = 0; i < PCIE_CONFIG_VAR_NUM_RB; i++)
        {
            /* Copy over the rb config */
            pcie_root_bridge_config* rb_config = &(get_rb_config->rootbridge_config[i]);

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

            printf("bus range 0x%lx - 0x%lx\n",
                   (unsigned long)rb_config->bus.base,
                   (unsigned long)rb_config->bus.limit);
        }
        memset((void*)SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE, 0, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_SIZE);
    }
    else if (context == &vab_cb_ctx)
    {
        /* Print VAB configurations */
        for (uint8_t i = 0; i < PCIE_CONFIG_VAR_NUM_VAB; i++)
        {
            printf("VAB %d smmu_bypass %d\n", i, (int)(get_vab_config->perf_config[i].smmu_bypass));
        }
        memset((void*)SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_BASE, 0, SCP_EXP_SCP_PCIE_VARIABLE_SERVICE_PAYLOAD_SIZE);
    }
}

static void get_hsp_variable(guid_t* guid_ptr, uint16_t* variable_name_ptr, size_t variable_name_size, size_t data_size, uint32_t* cb_ctx)
{
    uintptr_t target_addr = (uintptr_t)(SCP_EXP_SCP_VARIABLE_SERVICE_PAYLOAD_BASE);

    struct hsp_mbox_get_variable get_var;
    get_var.variable_name_size = variable_name_size / sizeof(uint16_t);
    get_var.vendor_guid.guid.data1 = guid_ptr->guid1;
    get_var.vendor_guid.guid.data2 = guid_ptr->guid2;
    get_var.vendor_guid.guid.data3 = guid_ptr->guid3;
    get_var.vendor_guid.guid.data4[0] = guid_ptr->guid4[0];
    get_var.vendor_guid.guid.data4[1] = guid_ptr->guid4[1];
    get_var.vendor_guid.guid.data4[2] = guid_ptr->guid4[2];
    get_var.vendor_guid.guid.data4[3] = guid_ptr->guid4[3];
    get_var.vendor_guid.guid.data4[4] = guid_ptr->guid4[4];
    get_var.vendor_guid.guid.data4[5] = guid_ptr->guid4[5];
    get_var.vendor_guid.guid.data4[6] = guid_ptr->guid4[6];
    get_var.vendor_guid.guid.data4[7] = guid_ptr->guid4[7];
    get_var.attributes_size = 0;
    get_var.data_size = data_size;

    memcpy((void*)target_addr, (void*)&get_var, sizeof(get_var));
    target_addr += sizeof(get_var);

    memcpy((void*)target_addr, (void*)variable_name_ptr, variable_name_size);
    target_addr += variable_name_size;

    get_rb_config = (kingsgate_pcie_root_bridge_config*)target_addr;
    get_vab_config = (kingsgate_pcie_vab_config*)target_addr;

    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)fpfw_init_get_handle("icc_hspmbx");

    get_var_send_req.cb_ctx = cb_ctx;
    fpfw_status_t send_status = fpfw_icc_base_send(icc_ctx, &get_var_send_req);
    printf("GetVariable request: got status %x\n", (int)send_status);

    get_var_recv_req.cb_ctx = cb_ctx;
    fpfw_status_t recv_status = fpfw_icc_base_recv(icc_ctx, &get_var_recv_req);
    printf("GetVariable response: got status %x\n", (int)recv_status);
}

static FPFW_CLI_STATUS dump_pci_hsp_variables(int argc, const char** argv)
{
    if (argc != 2)
    {
        return CLI_ERROR;
    }
    guid_t rb_config_guid_die_0 = PCIE_RB_CONFIG_VAR_DIE_0_GUID;
    guid_t vab_config_guid_die_0 = VAB_CONFIG_VAR_DIE_0_GUID;
    uint16_t rb_config_varname_die_0[] = PCIE_RB_CONFIG_VAR_DIE_0_NAME;
    uint16_t vab_config_varname_die_0[] = VAB_CONFIG_VAR_DIE_0_NAME;

    guid_t rb_config_guid_die_1 = PCIE_RB_CONFIG_VAR_DIE_1_GUID;
    guid_t vab_config_guid_die_1 = VAB_CONFIG_VAR_DIE_1_GUID;
    uint16_t rb_config_varname_die_1[] = PCIE_RB_CONFIG_VAR_DIE_1_NAME;
    uint16_t vab_config_varname_die_1[] = VAB_CONFIG_VAR_DIE_1_NAME;

    switch (strtoul(argv[1], NULL, 0))
    {
    case (1):
        get_hsp_variable(&rb_config_guid_die_0,
                         rb_config_varname_die_0,
                         sizeof(rb_config_varname_die_0),
                         sizeof(kingsgate_pcie_root_bridge_config),
                         &rb_cb_ctx);
        break;
    case (2):
        get_hsp_variable(&vab_config_guid_die_0,
                         vab_config_varname_die_0,
                         sizeof(vab_config_varname_die_0),
                         sizeof(kingsgate_pcie_vab_config),
                         &vab_cb_ctx);
        break;
    case (3):
        get_hsp_variable(&rb_config_guid_die_1,
                         rb_config_varname_die_1,
                         sizeof(rb_config_varname_die_1),
                         sizeof(kingsgate_pcie_root_bridge_config),
                         &rb_cb_ctx);
        break;
    case (4):
        get_hsp_variable(&vab_config_guid_die_1,
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
static FPFW_CLI_STATUS dump_rpss_entity(int argc, const char** argv)
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
        FpFwCliPrint("pcie cli: Failed to get entity from the PCIe driver! silibs_status: %d\n", (int)req.status);
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS dump_rp_entity(int argc, const char** argv)
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
        FpFwCliPrint("pcie cli: Failed to get entity from the PCIe driver! silibs_status: %d\n", (int)req.status);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS dump_rp_link_info(int argc, const char** argv)
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
        FpFwCliPrint("pcie cli: Failed to get ltssm state from the PCIe driver! silibs_status: %d\n", (int)req.status);
        return CLI_ERROR;
    }

    cli_op = GET_RP_LINK_STATE;
    req.p_requested_data = (void*)(&current_link_state);
    DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(&iface[rpss_idx]), &req.header);

    print_link_state(rpss_idx, rp_idx, ltssm_state, current_link_state);

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS dump_rp_dbi_cfg_hdr(int argc, const char** argv)
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
        FpFwCliPrint("pcie cli: Failed to get root port config from the PCIe driver! silibs_status: %d\n",
                     (int)req.status);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

void pcie_cli_init(pciess_device_t* pcie_dev_handles)
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

    FpFwCliRegisterTable(&pcie_cli_table[0], (sizeof(pcie_cli_table) / sizeof(FPFW_CLI_COMMAND)));
}
