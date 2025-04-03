//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_knobs.c
 * This file provides the functions to process the accelerator knobs and pass them to the accelerator.
 */

/*-------------------------------- Features ---------------------------------*/

/*-------------------------------- Includes ---------------------------------*/
#include <DbgPrint.h>          // for FPFW_DBGPRINT_INFO, FPFW_DBGPRINT_ERROR
#include <FpFwUtils.h>         // for FPFW_ARRAY_SIZE
#include <accelerator_ip.h>    // for scp_download_accel_knobs
#include <accelerator_knobs.h> // for transfer_accel_sys_info_data_t, knob_transfer_status_t
#include <atu_init.h>          // for atu_svc_accel_atu_addr
#include <bug_check.h>         // for BUG_ASSERT, BUG_CHECK
#include <config_manager.h> // for config_get_sdm_base_class_code, config_get_sdm_pf_subsystem_id, config_get_sdm_pf_device_id, config_get_sdm_pf_int_pin
#include <idhw.h>           // for idhw_get_die_id, idhw_get_platform_...
#include <idsw.h>           // for idsw_get_platform_sdv, idsw...
#include <stdint.h>         // for uint32_t
#include <string.h>         // for memcpy, strlen

/*-------------------- Symbolic Constant Macros (defines) -------------------*/
#define BYTES_IN_32B_WORD 4

// Transfer this to Cortex m7 once both sides of the implementation are complete: ADO 2503735
#define START_MAGIC_STRING "KNOBS_START"
#define END_MAGIC_STRING   "KNOBS_END"

#define STRING_BUFFER_LENGTH(x) (strlen(x) + 1) // +1 for NULL terminator

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

// Transfer this to Cortex m7 once both sides of the implementation are complete: ADO 2503735
const char* scp_start_magic_string = START_MAGIC_STRING;
const char* scp_end_magic_string = END_MAGIC_STRING;

// Transfer this to Cortex m7 once both sides of the implementation are complete: ADO 2503735
const uint32_t config_offset = 0x0007fc00;
const uint32_t config_size = 0x00000400;

// This is the list of knobs to be transferred to the accelerator. The list is yet to be finalized.
// For now, we are transferring the m7_test_knob to the accelerator. Remove later: ADO 2503735
const char* knob_list[] = {
    "m7_test_knob",
};

const uint32_t m7_test_knob_value = 0xFFFFFFFF;
// Clean up FPFW_DBGPRINT_ after all PRs in the series are merged : ADO 2503735

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/
static void intercore_aligned_memcpy(uint8_t* p_dest, uint8_t* p_src, uint16_t num_bytes, uint8_t align)
{
#ifdef _WIN32
    FPFW_UNUSED(p_dest);
    FPFW_UNUSED(p_src);
    FPFW_UNUSED(num_bytes);
    FPFW_UNUSED(align);
#else
    for (uint16_t i = 0; i < num_bytes; i++)
    {
        unsigned int align_bytes_offset = (unsigned int)p_dest % align;

        volatile uint32_t* p_dest_32b_aligned = (uint32_t*)(p_dest - align_bytes_offset);
        *p_dest_32b_aligned |= (*p_src) << (FPFW_BITS_IN_BYTE * align_bytes_offset);

        p_dest++;
        p_src++;
    }

#endif
}

static knob_transfer_status_t write_string_to_buff(uint8_t* p_addr, uint8_t* p_buff_end_addr, const char* p_str)
{
    size_t str_len = STRING_BUFFER_LENGTH(p_str);

    // Check that the buffer has enough space to write the str
    if (p_buff_end_addr < (p_addr + str_len))
    {
        return STATUS_KNOB_TRANSFER_FAIL_MEMORY_OVERFLOW;
    }

    intercore_aligned_memcpy(p_addr, (uint8_t*)p_str, str_len, BYTES_IN_32B_WORD);

    return STATUS_KNOB_TRANSFER_SUCCESS;
}

static void init_accel_sys_info(uint32_t* platform_id, uint8_t* die_id, uint32_t* soc_id)
{
    // Get the platform ID from IDSW
    *platform_id = idsw_get_platform_sdv();

    // Get the die ID from IDSW
    *die_id = idsw_get_die_id();

    // Get the SOC ID from IDHW
    *soc_id = idhw_get_soc_id();
}

static knob_transfer_status_t write_knob_to_buffer(uint8_t* p_addr,
                                                   uint8_t* p_buff_end_addr,
                                                   const char* p_knob_name,
                                                   uint8_t knob_size,
                                                   uint8_t* p_knob_data)
{
    if (p_knob_data == NULL)
    {
        return STATUS_KNOB_TRANSFER_FAIL_INVALID_KNOB_DATA;
    }

    // Check that the buffer has enough space to write the knob data
    if (p_buff_end_addr < (p_addr + STRING_BUFFER_LENGTH(p_knob_name) + sizeof(uint8_t) + knob_size))
    {
        return STATUS_KNOB_TRANSFER_FAIL_MEMORY_OVERFLOW;
    }

    // Copy the knob name into the buffer
    FPFW_DBGPRINT_INFO("Name: %s, Name Size: %d, ", p_knob_name, STRING_BUFFER_LENGTH(p_knob_name));
    intercore_aligned_memcpy(p_addr, (uint8_t*)p_knob_name, STRING_BUFFER_LENGTH(p_knob_name), BYTES_IN_32B_WORD);
    p_addr += STRING_BUFFER_LENGTH(p_knob_name);

    // Copy the size of the knob data into the buffer
    FPFW_DBGPRINT_INFO("Data Size: %d, Addr: %p, Dest Addr: %p \n", knob_size, &knob_size, p_addr);
    intercore_aligned_memcpy(p_addr, &knob_size, sizeof(uint8_t), BYTES_IN_32B_WORD);
    p_addr += sizeof(uint8_t);

    // Copy the knob data into the buffer
    intercore_aligned_memcpy(p_addr, p_knob_data, knob_size, BYTES_IN_32B_WORD);

    return STATUS_KNOB_TRANSFER_SUCCESS;
}

static knob_transfer_status_t transfer_accel_sys_info(uint8_t* p_addr, uint8_t* p_buff_end_addr, uint16_t* p_bytes_written)
{
    uint32_t accel_runtime_metadata_struct_size = 0;
    knob_transfer_status_t status;
    uint16_t bytes_written_temp = 0;

    static uint32_t platform_id;
    static uint8_t die_id;
    static uint32_t soc_id;

    static transfer_accel_sys_info_data_t accel_sys_info[] = {
        {"m7_sys_id", (uint8_t)(sizeof(uint8_t)), NULL},
        {"m7_soc_id", (uint8_t)(sizeof(uint32_t)), (uint8_t*)&soc_id},
        {"m7_die_id", (uint8_t)(sizeof(uint8_t)), (uint8_t*)&die_id},
        {"m7_plat_id", (uint8_t)(sizeof(uint32_t)), (uint8_t*)&platform_id},
    };

    init_accel_sys_info(&platform_id, &die_id, &soc_id);

    // Compute the size of the runtime data to be loaded into the accelerator
    for (uint32_t index = 0; index < FPFW_ARRAY_SIZE(accel_sys_info); index++)
    {
        accel_runtime_metadata_struct_size +=
            STRING_BUFFER_LENGTH(accel_sys_info[index].p_name) + sizeof(uint8_t) + accel_sys_info[index].size;
    }

    // Run an error check against available size
    if (p_addr + accel_runtime_metadata_struct_size > p_buff_end_addr)
    {
        FPFW_DBGPRINT_ERROR("Buffer too small for sys info data\n");
        return STATUS_KNOB_TRANSFER_FAIL_MEMORY_OVERFLOW;
    }

    // Write the runtime metadata to the buffer
    for (uint32_t index = 0; index < FPFW_ARRAY_SIZE(accel_sys_info); index++)
    {
        status = write_knob_to_buffer(p_addr,
                                      p_buff_end_addr,
                                      accel_sys_info[index].p_name,
                                      accel_sys_info[index].size,
                                      (uint8_t*)accel_sys_info[index].p_data);
        if (status != STATUS_KNOB_TRANSFER_SUCCESS)
        {
            FPFW_DBGPRINT_ERROR("Failed to write knob %s. Continuing . . .\n", accel_sys_info[index].p_name);
            continue;
        }
        bytes_written_temp +=
            STRING_BUFFER_LENGTH(accel_sys_info[index].p_name) + sizeof(uint8_t) + accel_sys_info[index].size;
        p_addr += STRING_BUFFER_LENGTH(accel_sys_info[index].p_name) + sizeof(uint8_t) + accel_sys_info[index].size;
    }

    (*p_bytes_written) = bytes_written_temp;
    return STATUS_KNOB_TRANSFER_SUCCESS;
}

static knob_transfer_status_t transfer_knob_to_accel(uint8_t* p_addr, uint8_t* p_buff_end_addr, const char* p_knob_name, uint16_t* p_bytes_written)
{
    knob_transfer_status_t status;

    // Search for the knob name in the cached knob data
    FPFW_DBGPRINT_INFO("Searching for knob: %s\n", p_knob_name);
    for (uint32_t knob_index = 0; knob_index < cached_knob_data_size(); knob_index++)
    {
        // Get the pointer to the current knob data
        cached_knob_data_t* cached_knob = &(get_cached_knob_data()[knob_index]);

        // Check if the knob name matches
        if (strcmp(cached_knob->name, p_knob_name) == 0)
        {
            FPFW_DBGPRINT_INFO("Found Knob. Index: %d, ", (int)knob_index);

            status = write_knob_to_buffer(p_addr,
                                          p_buff_end_addr,
                                          cached_knob->name,
                                          (uint8_t)cached_knob->size,
                                          (uint8_t*)cached_knob->data);
            if (status != STATUS_KNOB_TRANSFER_SUCCESS)
            {
                FPFW_DBGPRINT_ERROR("Failed to write knob %s\n", cached_knob->name);
                return status;
            }

            (*p_bytes_written) = STRING_BUFFER_LENGTH(cached_knob->name) + sizeof(uint8_t) + cached_knob->size;
            return STATUS_KNOB_TRANSFER_SUCCESS;
        }
    }
    return STATUS_KNOB_TRANSFER_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/

knob_transfer_status_t scp_download_accel_knobs(ACCEL_ID accel_type)
{
    knob_transfer_status_t status;
    uint16_t bytes_written = 0;
    uint8_t* p_config_data_start_address = (uint8_t*)atu_svc_accel_atu_addr(accel_type) + config_offset;
    uint8_t* p_config_data_end_address = p_config_data_start_address + config_size;

    BUG_ASSERT(p_config_data_start_address != NULL);
    BUG_ASSERT(p_config_data_end_address != NULL);

    uint8_t* p_config_data_write_address = p_config_data_start_address;

    // Leave space for the start magic string - this will be written at the end
    p_config_data_write_address += STRING_BUFFER_LENGTH(scp_start_magic_string);

    FPFW_DBGPRINT_INFO("Accel: Transfer config data for Accel %d, Die %d\n", accel_type, idsw_get_die_id());
    FPFW_DBGPRINT_INFO("Buffer Address: %p, End: %p\n", p_config_data_start_address, p_config_data_end_address);

    status = transfer_accel_sys_info(p_config_data_write_address, p_config_data_end_address, &bytes_written);
    if (status != STATUS_KNOB_TRANSFER_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("Failed to write sys info\n");
        return status;
    }
    p_config_data_write_address += bytes_written;

    for (uint32_t knob_num = 0; knob_num < FPFW_ARRAY_SIZE(knob_list); knob_num++)
    {
        BUG_ASSERT(knob_list[knob_num] != NULL);

        status = transfer_knob_to_accel(p_config_data_write_address, p_config_data_end_address, knob_list[knob_num], &bytes_written);
        if (status != STATUS_KNOB_TRANSFER_SUCCESS)
        {
            FPFW_DBGPRINT_ERROR("Failed to write knob data. Continuing . . .\n");
            continue;
        }

        p_config_data_write_address += bytes_written;
    }

    FPFW_DBGPRINT_INFO("Writing %s to the buffer at %p\n", scp_end_magic_string, p_config_data_write_address);
    status = write_string_to_buff(p_config_data_write_address, p_config_data_end_address, scp_end_magic_string);
    if (status != STATUS_KNOB_TRANSFER_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("Failed to write %s\n", scp_end_magic_string);
        return status;
    }

    p_config_data_write_address = p_config_data_start_address;
    FPFW_DBGPRINT_INFO("Writing %s to the buffer at %p\n", scp_start_magic_string, p_config_data_write_address);
    status = write_string_to_buff(p_config_data_start_address, p_config_data_end_address, scp_start_magic_string);
    if (status != STATUS_KNOB_TRANSFER_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("Failed to write %s\n", scp_start_magic_string);
        return status;
    }

    return STATUS_KNOB_TRANSFER_SUCCESS;
}