//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file  hm_cli_main.c
 * This file contains the implementation of the  health monitor CLI interface
 * and related functionality.
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <einj.h>
#include <health_monitor.h>
#include <hm_cli.h>
#include <string.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
#pragma pack(push, 1)
typedef struct _hm_timestamp_t
{
    uint8_t sec;
    uint8_t minute;
    uint8_t hour;
    uint8_t precise;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t century;
} hm_timestamp_t;
#pragma pack(pop)

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS hm_dump_ghes_cli(int argc, const char** argv);
static FPFW_CLI_STATUS hm_dump_einj_cli(int argc, const char** argv);
static FPFW_CLI_STATUS hm_dump_last_cper_cli(int argc, const char** argv);
static FPFW_CLI_STATUS hm_inject_err_cli(int argc, const char** argv);
static FPFW_CLI_STATUS hm_inject_err_raw_cli(int argc, const char** argv);
static FPFW_CLI_STATUS hm_activate_sample_err_domain_cli(int argc, const char** argv);
static FPFW_CLI_STATUS hm_submit_sample_cper_cli(int argc, const char** argv);
static void dump_ghes(uint32_t ghes_idx);
static void dump_ghes_error_record(acpi_ghes_error_record_dual_die_t* ghes_error_record_base, uint32_t max_section_count);
static void print_section_as_byte_view(const acpi_cper_section_t* section);
static void print_einj_payload(ras_einj_info_t* payload);
static bool check_memory_corruption(void* start1, uint32_t size1, void* start2, uint32_t size2, void* start3, uint32_t size3);
static const char* get_section_name(guid_t section_type);
static acpi_einj_cmd_status_t hm_cli_error_injection_cb(ras_einj_info_t* payload, void* ctx);

/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND cfg_mgr_cli_list[] = {
    {NULL_LIST_ENTRY, "hm", "hm_dump_ghes", hm_dump_ghes_cli, "dump ghes {err_idx}", ""},
    {NULL_LIST_ENTRY, "hm", "hm_dump_einj", hm_dump_einj_cli, "dump einj payload", ""},
    {NULL_LIST_ENTRY, "hm", "hm_dump_last_cper", hm_dump_last_cper_cli, "dump last full cper record", ""},
    {NULL_LIST_ENTRY, "hm", "hm_inject_err", hm_inject_err_cli, "inject err {err_idx}", ""},
    {NULL_LIST_ENTRY, "hm", "hm_inject_err_raw", hm_inject_err_raw_cli, "inject err {err_idx}", ""},
    {NULL_LIST_ENTRY, "hm", "hm_activate_sample_domain", hm_activate_sample_err_domain_cli, "active sample err domain", ""},
    {NULL_LIST_ENTRY, "hm", "hm_submit_sample_cper", hm_submit_sample_cper_cli, "submit ce cper {err_idx}", ""}};

static const guid_name_map_t guid_map[] = {
    {ACPI_ERROR_TYPE_VENDOR_DDR, "ddr mem"},
    {ACPI_ERROR_TYPE_VENDOR_MESH_NODE, "mesh node"},
    {ACPI_ERROR_TYPE_PLATFORM_MEMORY, "platform mem"},
    {ACPI_ERROR_TYPE_VENDOR_FIRMWARE, "vendor f/w"},
    {ACPI_ERROR_TYPE_PROCESSOR_GENERIC, "proc generic"},
    {ACPI_ERROR_TYPE_VENDOR_AP, "AP"},
    {ACPI_ERROR_TYPE_VENDOR_PCIE, "vendor pcie"},
    {ACPI_ERROR_TYPE_PROCESSOR_ARM, "proc arm"},
    {ACPI_ERROR_TYPE_VENDOR_HSP_PROCESSOR, "hsp"},
    {ACPI_ERROR_TYPE_VENDOR_PEX, "pex"},
    {ACPI_ERROR_TYPE_VENDOR_RHTLM, "RowHammer"},
};

/*------------- Functions ----------------*/
static PLACED_CODE FPFW_CLI_STATUS hm_dump_ghes_cli(int argc, const char** argv)
{
    int ghes_idx = -1;

    if (argc == 2)
    {
        ghes_idx = atoi(argv[1]);

        if (ghes_idx < 0 || ghes_idx >= ACPI_ERROR_DOMAIN_COUNT)
        {
            FpFwCliPrint("ERR:Check Args(%d)\n", __LINE__);
            return CLI_ERROR;
        }

        dump_ghes(ghes_idx);
    }
    else
    {
        for (ghes_idx = 0; ghes_idx < ACPI_ERROR_DOMAIN_COUNT; ghes_idx++)
        {
            dump_ghes(ghes_idx);
        }
    }

    // dump the error record addr table
    hm_config_t* hm_config = get_hm_config();
    BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);

    FpFwCliPrint("Err Record Addr Table Info\n");
    uint64_t* err_record_table_addr = (uint64_t*)hm_config->mscp_ghes_error_record_addr_table_base;
    for (uint32_t error_domain_idx = 0; error_domain_idx < ACPI_ERROR_DOMAIN_COUNT; error_domain_idx++)
    {
        FpFwCliPrint(" err domain idx: %d at %p\n", error_domain_idx, (void*)(uintptr_t)(err_record_table_addr[error_domain_idx]));
    }
    FpFwCliPrint("\n");

    // dump the ack addr table
    FpFwCliPrint("ACK addr \n");
    uint64_t* err_record_ack_addr = (uint64_t*)hm_config->mscp_ghes_ack_addr_table_base;
    for (uint32_t error_domain_idx = 0; error_domain_idx < ACPI_ERROR_DOMAIN_COUNT; error_domain_idx++)
    {
        FpFwCliPrint(" err domain idx: %d at %p\n", error_domain_idx, (void*)(err_record_ack_addr++));
    }
    FpFwCliPrint("\n");

    // calculate the total size of GHES and Error record
    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base = NULL;
    for (uint32_t error_domain_idx = 0; error_domain_idx < ACPI_ERROR_DOMAIN_COUNT; error_domain_idx++)
    {
        uint32_t error_record_base_addr = MSCP_GHES_ADDR((uint32_t)current_ghes_base->address.address);
        uint32_t error_record_base = MSCP_GHES_ADDR(*(uint32_t*)error_record_base_addr);

        current_ghes_error_record_base =
            (acpi_ghes_error_record_dual_die_t*)(error_record_base + hm_config->mscp_ghes_base_apcore_offset);

        current_ghes_base++;
    }

    FpFwCliPrint("Total GHES size = %d\n", ((uint32_t)current_ghes_base - (uint32_t)hm_config->mscp_ghes_base));
    FpFwCliPrint("Total Err record size = %d\n",
                 ((uint32_t)current_ghes_error_record_base - (uint32_t)hm_config->mscp_ghes_error_record_addr_base));
    FpFwCliPrint("GHES entity size:%d\n", sizeof(acpi_ghes_t));
    FpFwCliPrint("Err record size :%d\n", sizeof(acpi_ghes_error_record_dual_die_t));
    FpFwCliPrint("Err record section size :%d\n", sizeof(acpi_cper_section_t));

    // check any memory corruption between GHES, Error record and Ack addr table
    if (check_memory_corruption((void*)hm_config->mscp_ghes_base,
                                (uint32_t)current_ghes_base - (uint32_t)hm_config->mscp_ghes_base,
                                (void*)hm_config->mscp_ghes_error_record_addr_base,
                                (uint32_t)current_ghes_error_record_base - (uint32_t)hm_config->mscp_ghes_error_record_addr_base,
                                (void*)hm_config->mscp_ghes_ack_addr_table_base,
                                sizeof(uint64_t) * ACPI_ERROR_DOMAIN_COUNT) == false)
    {
        BUG_ASSERT_PARAM(false, hm_config->mscp_ghes_base, hm_config->mscp_ghes_error_record_addr_base);
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS hm_inject_err_cli(int argc, const char** argv)
{
    if (argc < 9)
    {
        FpFwCliPrint("usage: hm_inject_err <group> <type> <instance> <status_operation> <err_type> "
                     "<err_severity> <err_addr> <err_value>\n");
        return CLI_ERROR;
    }

    ras_einj_info_t input_einj_payload;
    memset(&input_einj_payload, 0, sizeof(ras_einj_info_t));
    input_einj_payload.version = ERROR_INJECTION_PAYLOAD_VERSION;

    input_einj_payload.component_group = (uint16_t)strtol(argv[1], NULL, 0);
    input_einj_payload.component_type = (uint16_t)strtol(argv[2], NULL, 0);
    input_einj_payload.component_instance = (uint16_t)strtol(argv[3], NULL, 0);
    input_einj_payload.status_operation.value = (uint16_t)strtol(argv[4], NULL, 0);
    input_einj_payload.param_type.error_type = (uint16_t)strtol(argv[5], NULL, 0);
    input_einj_payload.param_type.severity = (uint16_t)strtol(argv[6], NULL, 0);
    input_einj_payload.param_type.address_64 = (uint64_t)strtoull(argv[7], NULL, 0);
    input_einj_payload.value_type.error_values = (uint64_t)strtoull(argv[8], NULL, 0);

    print_einj_payload(&input_einj_payload);

    hm_config_t* hm_config = get_hm_config();
    BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);

    volatile ras_einj_info_t* einj_payload = (ras_einj_info_t*)hm_config->mscp_error_injection_addr_base;

    for (uint32_t i = 0; i < sizeof(ras_einj_info_t); i++)
    {
        ((volatile uint8_t*)einj_payload)[i] = ((const uint8_t*)&input_einj_payload)[i];
    }

    acpi_einj_cmd_status_t einj_result = ACPI_EINJ_UNKNOWN_FAILURE;
    einj_result = hm_inject_error();
    if (einj_result != ACPI_EINJ_SUCCESS)
    {
        FpFwCliPrint("EINJ Failed(%d)\n", einj_result);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS hm_inject_err_raw_cli(int argc, const char** argv)
{
    if (argc < 9)
    {
        FpFwCliPrint("usage: hm_inject_err_raw_cli <group> <type> <instance> <status> <operation> "
                     "<error_parameter_0> <error_parameter_1> <error_value>\n");
        return CLI_ERROR;
    }

    ras_einj_info_t input_einj_payload;
    memset(&input_einj_payload, 0, sizeof(ras_einj_info_t));
    input_einj_payload.version = ERROR_INJECTION_PAYLOAD_VERSION;

    input_einj_payload.component_group = (uint16_t)strtol(argv[1], NULL, 0);
    input_einj_payload.component_type = (uint16_t)strtol(argv[2], NULL, 0);
    input_einj_payload.component_instance = (uint16_t)strtol(argv[3], NULL, 0);
    input_einj_payload.status_operation.status = (uint8_t)strtol(argv[4], NULL, 0);
    input_einj_payload.status_operation.operation = (uint8_t)strtol(argv[5], NULL, 0);
    input_einj_payload.param_type.error_parameters[0] = (uint64_t)strtoull(argv[6], NULL, 0);
    input_einj_payload.param_type.error_parameters[1] = (uint64_t)strtoull(argv[7], NULL, 0);
    input_einj_payload.value_type.error_values = (uint64_t)strtoull(argv[8], NULL, 0);

    print_einj_payload(&input_einj_payload);

    hm_config_t* hm_config = get_hm_config();
    BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);

    volatile ras_einj_info_t* einj_payload = (ras_einj_info_t*)hm_config->mscp_error_injection_addr_base;

    for (uint32_t i = 0; i < sizeof(ras_einj_info_t); i++)
    {
        ((volatile uint8_t*)einj_payload)[i] = ((const uint8_t*)&input_einj_payload)[i];
    }

    acpi_einj_cmd_status_t einj_result = ACPI_EINJ_UNKNOWN_FAILURE;
    einj_result = hm_inject_error();
    if (einj_result != ACPI_EINJ_SUCCESS)
    {
        FpFwCliPrint("EINJ Failed(%d)\n", einj_result);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS hm_dump_einj_cli(int argc, const char** argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(argv);

    hm_config_t* hm_config = get_hm_config();
    BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);

    ras_einj_info_t* einj_payload = (ras_einj_info_t*)hm_config->mscp_error_injection_addr_base;

    print_einj_payload(einj_payload);
    return CLI_SUCCESS;
}

static PLACED_CODE void PrintOutGuid(const char* name, guid_t guid)
{
    FpFwCliPrint("%s: {0x%08X, 0x%04X, 0x%04X, {0x%02X, 0x%02X, 0x%02X, 0x%02X, "
                 "0x%02X, 0x%02X, 0x%02X, 0x%02X}}\n",
                 name,
                 guid.guid1,
                 guid.guid2,
                 guid.guid3,
                 guid.guid4[0],
                 guid.guid4[1],
                 guid.guid4[2],
                 guid.guid4[3],
                 guid.guid4[4],
                 guid.guid4[5],
                 guid.guid4[6],
                 guid.guid4[7]);
}

static PLACED_CODE void dump_cper_contents(acpi_cper_record_t* record_addr)
{
    if (!record_addr)
    {
        return;
    }

    acpi_cper_record_header_t* hdr = &record_addr->record_header;
    FpFwCliPrint("CPER Record Header:\n");
    FpFwCliPrint("  Signature: %c%c%c%c\n",
                 (hdr->signature_start & 0xFF),
                 (hdr->signature_start >> 8) & 0xFF,
                 (hdr->signature_start >> 16) & 0xFF,
                 (hdr->signature_start >> 24) & 0xFF);

    FpFwCliPrint("  Revision: %u\n", hdr->revision);
    FpFwCliPrint("  Section Count: %u\n", hdr->section_count);
    FpFwCliPrint("  Error Severity: 0x%08x\n", hdr->error_severity);
    FpFwCliPrint("  Record Length: %u\n", hdr->record_length);
    PrintOutGuid("  Platform_id", hdr->platform_id);
    PrintOutGuid("  Creator_id", hdr->creator_id);
    PrintOutGuid("  Notification_type", hdr->notification_type);

    uint8_t* base = (uint8_t*)record_addr;
    for (int i = 0; i < hdr->section_count; ++i)
    {
        acpi_cper_sec_desc_t* desc =
            (acpi_cper_sec_desc_t*)(base + sizeof(acpi_cper_record_header_t) + i * sizeof(acpi_cper_sec_desc_t));
        FpFwCliPrint("\nSection Descriptor %d:\n", i);
        FpFwCliPrint("  Section Offset: %u\n", desc->section_offset);
        FpFwCliPrint("  Section Length: %u\n", desc->section_length);
        FpFwCliPrint("  Revision: %u\n", desc->revision);
        PrintOutGuid("  Section Type", desc->section_type);
        PrintOutGuid("  FRU ID", desc->fru_id);
        FpFwCliPrint("  Severity: 0x%08x\n", desc->section_severity);
        FpFwCliPrint("  FRU Text: %.*s\n", ACPI_FRU_TEXT_LENGTH, desc->fru_text);

        uint8_t* section_data = base + desc->section_offset;
        FpFwCliPrint("  Section Data : ");
        for (uint32_t j = 0; j < desc->section_length; ++j)
        {
            FpFwCliPrint("%02x ", section_data[j]);
        }
        FpFwCliPrint("\n");
    }
}

static PLACED_CODE FPFW_CLI_STATUS hm_dump_last_cper_cli(int argc, const char** argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(argv);

    hm_config_t* hm_config = get_hm_config();
    BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);

    acpi_cper_record_t* cper_record_on_rmss = (acpi_cper_record_t*)hm_config->mscp_full_cper_record_base;
    dump_cper_contents(cper_record_on_rmss);

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS hm_activate_sample_err_domain_cli(int argc, const char** argv)
{
    const char* err_domain_name = NULL;
    uint32_t err_domain_idx = ACPI_ERROR_DOMAIN_MESH;

    if (argc == 2)
    {
        err_domain_name = argv[1];
        hm_register_error_domain(err_domain_idx, NULL, err_domain_name, hm_cli_error_injection_cb, NULL);
    }
    else
    {
        FpFwCliPrint("ERR:Check Args(%d)\n", __LINE__);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS hm_submit_sample_cper_cli(int argc, const char** argv)
{
    acpi_error_domain_t err_domain_idx = ACPI_ERROR_DOMAIN_INVALID;

    if (argc == 2)
    {
        int err_idx = atoi(argv[1]);

        if (err_idx < ACPI_ERROR_DOMAIN_DDR || err_idx > ACPI_ERROR_DOMAIN_COUNT - 1)
        {
            FpFwCliPrint("ERR:Invalid Error Domain(%d)\n", __LINE__);
            return CLI_ERROR;
        }

        err_domain_idx = (acpi_error_domain_t)err_idx;
    }
    else
    {
        FpFwCliPrint("ERR:Check Args(%d)\n", __LINE__);
        return CLI_ERROR;
    }

    acpi_cper_section_t general_cper_section = {0};

    hm_submit_cper(err_domain_idx, ACPI_ERROR_SEVERITY_INFORMATIONAL, &general_cper_section, sizeof(acpi_cper_section_t));
    return CLI_SUCCESS;
}

void dump_ghes(uint32_t ghes_idx)
{
    hm_config_t* hm_config = get_hm_config();
    BUG_ASSERT_PARAM(hm_config != NULL, hm_config, 0);

    // locate target GHES entry
    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    current_ghes_base += ghes_idx;

    FpFwCliPrint("GHES (%d): addr: %p\n", (int)ghes_idx, (void*)current_ghes_base);
    FpFwCliPrint(" type:%d\n", current_ghes_base->type);
    FpFwCliPrint(" src_id:%d\n", current_ghes_base->source_id);
    FpFwCliPrint(" rel_src_id:0x%X\n", current_ghes_base->related_source_id);
    FpFwCliPrint(" flags:%02X\n", current_ghes_base->flags);
    FpFwCliPrint(" enable:%d\n", current_ghes_base->enabled);
    FpFwCliPrint(" num_rec:%d\n", current_ghes_base->num_of_records);
    FpFwCliPrint(" max_sec:%d\n", current_ghes_base->max_sections_per_record);
    FpFwCliPrint(" max_raw_len:%d\n", current_ghes_base->max_raw_data_length);
    FpFwCliPrint(" err_block_len:%d\n", current_ghes_base->error_status_block_length);
    FpFwCliPrint(" addr.id:%d\n", current_ghes_base->address.space_id);
    FpFwCliPrint(" addr.width:%d\n", current_ghes_base->address.reg_bit_width);
    FpFwCliPrint(" addr.offset:%d\n", current_ghes_base->address.reg_bit_offset);
    FpFwCliPrint(" addr.size:%d\n", current_ghes_base->address.access_size);
    FpFwCliPrint(" addr.addr:%p\n", (void*)(uintptr_t)current_ghes_base->address.address);
    FpFwCliPrint(" noti.type:%d\n", current_ghes_base->notification.type);
    FpFwCliPrint(" noti.len:%d\n", current_ghes_base->notification.length);
    FpFwCliPrint(" noti.cfg_type:%d\n", current_ghes_base->notification.cfg_wr_enable_type);
    FpFwCliPrint(" noti.cfg_interval:%d\n", current_ghes_base->notification.cfg_wr_enable_poll_interval);
    FpFwCliPrint(" noti.cfg_thrs_val:%d\n", current_ghes_base->notification.cfg_wr_enable_sw_poll_thrs_val);
    FpFwCliPrint(" noti.cfg_thrs_win:%d\n", current_ghes_base->notification.cfg_wr_enable_sw_poll_thrs_win);
    FpFwCliPrint(" noti.cfg_err_thrs_val:%d\n", current_ghes_base->notification.cfg_wr_enable_err_thrs_val);
    FpFwCliPrint(" noti.cfg_err_thrs_win:%d\n", current_ghes_base->notification.cfg_wr_enable_err_thrs_win);
    FpFwCliPrint(" noti.poll_interval:%d\n", current_ghes_base->notification.poll_interval);
    FpFwCliPrint(" noti.vec:%d\n", current_ghes_base->notification.vector);
    FpFwCliPrint(" noti.sw_th_val:%d\n", current_ghes_base->notification.sw_to_polling_threshold_value);
    FpFwCliPrint(" noti.sw__win:%d\n", current_ghes_base->notification.sw_to_polling_threshold_window);
    FpFwCliPrint(" noti.err_val:%d\n", current_ghes_base->notification.error_threshold_value);
    FpFwCliPrint(" noti.err_th_win:%d\n", current_ghes_base->notification.error_threashold_window);
    FpFwCliPrint(" ack.addr.id:%d\n", current_ghes_base->ack.address.space_id);
    FpFwCliPrint(" ack.addr.width:%d\n", current_ghes_base->ack.address.reg_bit_width);
    FpFwCliPrint(" ack.addr.offset:%d\n", current_ghes_base->ack.address.reg_bit_offset);
    FpFwCliPrint(" ack.addr.size:%d, addr:0x%08X\n",
                 current_ghes_base->ack.address.access_size,
                 current_ghes_base->ack.address.address);
    FpFwCliPrint(" ack.rd_ack_pre:0x%08X, Wr:0x%08X\n\n",
                 current_ghes_base->ack.rd_ack_preserve,
                 current_ghes_base->ack.rd_ack_wr);

    // Check if the GHES memory is corrupted
    if ((current_ghes_base->type != HM_GHES_VERSION_V2) || (current_ghes_base->source_id != ghes_idx) ||
        (current_ghes_base->num_of_records != HM_ERROR_RECORD_COUNT) ||
        (current_ghes_base->max_sections_per_record != HM_ERROR_SECTION_COUNT))
    {
        BUG_ASSERT_PARAM(false, current_ghes_base->num_of_records, current_ghes_base->max_sections_per_record);
    }

    // error record populate via GHES
    uint32_t error_record_base_addr = MSCP_GHES_ADDR((uint32_t)current_ghes_base->address.address);
    uint32_t error_record_base = MSCP_GHES_ADDR(*(uint32_t*)error_record_base_addr);

    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)(error_record_base + hm_config->mscp_ghes_base_apcore_offset);

    // Dump Error record associated with current GHES
    dump_ghes_error_record(current_ghes_error_record_base, current_ghes_base->max_sections_per_record);

    // Check if the Error Record memory is corrupted
    if ((current_ghes_error_record_base->data_length != sizeof(acpi_ghes_error_entry_t) * HM_ERROR_SECTION_COUNT))
    {
        BUG_ASSERT_PARAM(false, current_ghes_error_record_base->data_length, sizeof(acpi_ghes_error_entry_t) * HM_ERROR_SECTION_COUNT);
    }

    // Check if the Error Record Section memory is corrupted
    for (uint32_t section_count = 0; section_count < current_ghes_base->max_sections_per_record; section_count++)
    {
        if ((current_ghes_error_record_base->data[section_count].revision != 0x300) ||
            (current_ghes_error_record_base->data[section_count].error_data_length != sizeof(acpi_cper_section_t)))
        {
            BUG_ASSERT_PARAM(false,
                             current_ghes_error_record_base->data[section_count].error_data_length,
                             sizeof(acpi_cper_section_t));
        }
    }

    FpFwCliPrint("\n");
}

void dump_ghes_error_record(acpi_ghes_error_record_dual_die_t* ghes_error_record_base, uint32_t max_section_count)
{
    FpFwCliPrint("Err block(%p):\n", ghes_error_record_base);
    FpFwCliPrint(" UE=%d, CE=%d, Mul_UE=%d, Mul_CE=%d, Ctn=%d\n",
                 ghes_error_record_base->block_status_ue,
                 ghes_error_record_base->block_status_ce,
                 ghes_error_record_base->block_status_multi_ue,
                 ghes_error_record_base->block_status_multi_ce,
                 ghes_error_record_base->block_status_entry_count);
    FpFwCliPrint(" raw_dat_offset:%d\n", ghes_error_record_base->raw_data_offset);
    FpFwCliPrint(" raw_dat_len:%d\n", ghes_error_record_base->raw_data_length);
    FpFwCliPrint(" data_len:%d\n", ghes_error_record_base->data_length);
    FpFwCliPrint(" err_sev:%d\n", ghes_error_record_base->error_severity);

    for (uint32_t section_count = 0; section_count < max_section_count; section_count++)
    {
        // Dump Error entities in current error record
        FpFwCliPrint("Err@(%p):\n", (int)section_count, &(ghes_error_record_base->data[section_count]));
        FpFwCliPrint(" type: {0x%08X, 0x%04X, 0x%04X, {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, "
                     "0x%02X, 0x%02X, 0x%02X}}\n",
                     ghes_error_record_base->data[section_count].section_type.guid1,
                     ghes_error_record_base->data[section_count].section_type.guid2,
                     ghes_error_record_base->data[section_count].section_type.guid3,
                     ghes_error_record_base->data[section_count].section_type.guid4[0],
                     ghes_error_record_base->data[section_count].section_type.guid4[1],
                     ghes_error_record_base->data[section_count].section_type.guid4[2],
                     ghes_error_record_base->data[section_count].section_type.guid4[3],
                     ghes_error_record_base->data[section_count].section_type.guid4[4],
                     ghes_error_record_base->data[section_count].section_type.guid4[5],
                     ghes_error_record_base->data[section_count].section_type.guid4[6],
                     ghes_error_record_base->data[section_count].section_type.guid4[7]);
        FpFwCliPrint(" type: %s", get_section_name(ghes_error_record_base->data[section_count].section_type));
        FpFwCliPrint(" err_sev:%d\n", ghes_error_record_base->data[section_count].error_severity);
        FpFwCliPrint(" revision:%d\n", ghes_error_record_base->data[section_count].revision);
        FpFwCliPrint(" valid_id:%d\n", ghes_error_record_base->data[section_count].valid_fru);
        FpFwCliPrint(" valid_name:%d\n", ghes_error_record_base->data[section_count].valid_fru_str);
        FpFwCliPrint(" valid_time:%d\n", ghes_error_record_base->data[section_count].valid_fru_timestamp);
        FpFwCliPrint(" data.flags:%d\n", ghes_error_record_base->data[section_count].flags);
        FpFwCliPrint(" data.err_dat_len:%d\n", ghes_error_record_base->data[section_count].error_data_length);

        if (ghes_error_record_base->data[section_count].valid_fru)
        {
            FpFwCliPrint(" id: {0x%08X, 0x%04X, 0x%04X, {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, "
                         "0x%02X, 0x%02X, 0x%02X}}\n",
                         ghes_error_record_base->data[section_count].fru_id.guid1,
                         ghes_error_record_base->data[section_count].fru_id.guid2,
                         ghes_error_record_base->data[section_count].fru_id.guid3,
                         ghes_error_record_base->data[section_count].fru_id.guid4[0],
                         ghes_error_record_base->data[section_count].fru_id.guid4[1],
                         ghes_error_record_base->data[section_count].fru_id.guid4[2],
                         ghes_error_record_base->data[section_count].fru_id.guid4[3],
                         ghes_error_record_base->data[section_count].fru_id.guid4[4],
                         ghes_error_record_base->data[section_count].fru_id.guid4[5],
                         ghes_error_record_base->data[section_count].fru_id.guid4[6],
                         ghes_error_record_base->data[section_count].fru_id.guid4[7]);
        }

        if (ghes_error_record_base->data[section_count].valid_fru_str)
        {
            FpFwCliPrint(" name:%s\n", ghes_error_record_base->data[section_count].fru_text);
        }

        if (ghes_error_record_base->data[section_count].valid_fru_timestamp)
        {
            FpFwCliPrint(" timestamp:\n");
            hm_timestamp_t* p_timestamp = (hm_timestamp_t*)(&ghes_error_record_base->data[section_count].timestamp);

            if (p_timestamp->precise != 0)
            {
                FpFwCliPrint("Precise ");
            }

            FpFwCliPrint(" cen:yr:mn:dd:hh:mn:sec : %d:%d:%d:%d:%d:%d:%d:\n",
                         p_timestamp->century,
                         p_timestamp->year,
                         p_timestamp->month,
                         p_timestamp->day,
                         p_timestamp->hour,
                         p_timestamp->minute,
                         p_timestamp->sec);
        }

        print_section_as_byte_view(&ghes_error_record_base->data[section_count].section);
    }
}

void hm_cli_init(void)
{
    FpFwCliRegisterTable(&cfg_mgr_cli_list[0], FPFW_ARRAY_SIZE(cfg_mgr_cli_list));
}

bool check_memory_corruption(void* start1, uint32_t size1, void* start2, uint32_t size2, void* start3, uint32_t size3)
{
    uintptr_t end1 = (uintptr_t)start1 + size1;
    uintptr_t end2 = (uintptr_t)start2 + size2;
    uintptr_t end3 = (uintptr_t)start3 + size3;

    if ((uintptr_t)start1 < end2 && end1 > (uintptr_t)start2)
    {
        return false;
    }

    if ((uintptr_t)start1 < end3 && end1 > (uintptr_t)start3)
    {
        return false;
    }

    if ((uintptr_t)start2 < end3 && end2 > (uintptr_t)start3)
    {
        return false;
    }

    return true;
}

void print_section_as_byte_view(const acpi_cper_section_t* section)
{
    const uint8_t* section_data = (const uint8_t*)section;
    size_t section_size = sizeof(acpi_cper_section_t);

    FpFwCliPrint("[Byte View]");

    for (size_t dataIdx = 0; dataIdx < section_size; dataIdx++)
    {
        if ((dataIdx % 16) == 0)
        {
            FpFwCliPrint("\n\t\t");
        }
        else
        {
            FpFwCliPrint(" ");
        }

        FpFwCliPrint("%02x", section_data[dataIdx]);
    }
    FpFwCliPrint("\n");
}

void print_einj_payload(ras_einj_info_t* payload)
{
    if (payload == NULL)
    {
        return;
    }

    FpFwCliPrint("EINJ Payload\n");
    FpFwCliPrint(" einj version %ld\n", payload->version);
    FpFwCliPrint(" einj group %ld\n", payload->component_group);
    FpFwCliPrint(" einj type %ld\n", payload->component_type);
    FpFwCliPrint(" einj instance %ld\n", payload->component_instance);
    FpFwCliPrint(" einj stat_op.value %d\n", payload->status_operation.value);
    FpFwCliPrint(" einj stat_op.status %d\n", payload->status_operation.value & 0xFF);
    FpFwCliPrint(" einj stat_op.operation %d\n", (payload->status_operation.value >> 8) & 0xFF);
    FpFwCliPrint(" einj param_type.error_parameters[0] %d\n", (uint64_t)((payload->param_type.error_parameters[0])));
    FpFwCliPrint(" einj param_type.error_parameters[1] %d\n", (uint64_t)((payload->param_type.error_parameters[1])));
    FpFwCliPrint(" einj param_type.err_type %d\n", (uint16_t)((payload->param_type.error_parameters[0]) & 0xFFFF));
    FpFwCliPrint(" einj param_type.sev %d\n", (uint16_t)((payload->param_type.error_parameters[0] >> 16) & 0xFFFF));
    FpFwCliPrint(" einj param_type.reserved %d\n", (uint16_t)((payload->param_type.error_parameters[0] >> 32) & 0xFFFFFFFF));
    FpFwCliPrint(" einj param_type.addr64 0x%llx\n", payload->param_type.address_64);
    FpFwCliPrint(" einj param_type.addr32 0x%08x\n", payload->param_type.address_32);
    FpFwCliPrint(" einj value_type.error_values 0x%llx\n", payload->value_type.error_values);
    FpFwCliPrint(" einj value_type.data_64 0x%llx\n", payload->value_type.data_64);
    FpFwCliPrint(" einj value_type.data_32 0x%08x\n", payload->value_type.data_32);
}

const char* get_section_name(guid_t section_type)
{
    for (size_t i = 0; i < sizeof(guid_map) / sizeof(guid_name_map_t); i++)
    {
        if (memcmp(&section_type, &guid_map[i].guid, sizeof(guid_t)) == 0)
        {
            return guid_map[i].name;
        }
    }
    return "unknown";
}

acpi_einj_cmd_status_t hm_cli_error_injection_cb(ras_einj_info_t* payload, void* ctx)
{
    FPFW_UNUSED(payload);
    FPFW_UNUSED(ctx);

    return ACPI_EINJ_SUCCESS;
}