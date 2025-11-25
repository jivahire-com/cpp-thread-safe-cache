//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file advlog_parse.c
 *       API implementations for parsing advanced logger logs from DRAM using
 *       memapi.
 */

/*------------- Includes -----------------*/
#include "ap_advlog_parse.h"

#include <DbgPrint.h>
#include <FpFwAssert.h>
#include <atu_lib.h>
#include <bug_check.h>
#include <ddrss_reserved_regions.h>
#include <fpfw_status.h>
#include <silibs_platform.h>
#include <zlib.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/
static advanced_logger_info info;
static atu_map_entry_t ap_advlog_buffer_atu_map_struct;
static bool atu_mapped = false;

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/
bool populate_advanced_logger_info()
{
    int status = SILIBS_SUCCESS;
    volatile void* dest = &info;

    if (atu_mapped == false)
    {
        atu_entry_attr_t ap_advlog_buffer_atu_ns_attr = {ATU_BUS_ATTR_NS};

        ap_advlog_buffer_atu_map_struct.ap_base_address = AP_ADV_LOGGER_BUFFER_BASE;
        ap_advlog_buffer_atu_map_struct.mscp_start_address = 0;
        ap_advlog_buffer_atu_map_struct.mscp_end_address = AP_ADV_LOGGER_BUFFER_SIZE - 1;
        ap_advlog_buffer_atu_map_struct.attribute.as_uint32 = ap_advlog_buffer_atu_ns_attr.as_uint32;

        status = atu_map(ATU_ID_MSCP, &ap_advlog_buffer_atu_map_struct);
        BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

        FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Advanced Logger buffer mapped");

        atu_mapped = true;
    }

    for (size_t idx = 0; idx < sizeof(advanced_logger_info); idx++)
    {
        uint8_t byte = MMIO_READ8(ap_advlog_buffer_atu_map_struct.mscp_start_address + idx);
        *((uint8_t*)dest + idx) = byte;
    }

    if (info.signature != ADVANCED_LOGGER_SIGNATURE)
    {
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Logger signature mismatch! Expected signature 0x%lx, got "
                            "0x%lx - payload will not be sent!\n",
                            ADVANCED_LOGGER_SIGNATURE,
                            info.signature);
        return false;
    }

    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Log Buffer = 0x%lx\n", info.log_buffer);
    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Log Current = 0x%lx\n", info.log_current);

    return true;
}

uint64_t get_advanced_logger_base()
{
    return (ap_advlog_buffer_atu_map_struct.mscp_start_address);
}

uint64_t get_advanced_logger_size()
{
    uint64_t current = info.log_current;
    uint64_t base = info.log_buffer;
    uint64_t log_size = 0x00;

    if (current >= base)
    {
        if ((current - base) > AP_ADV_LOGGER_BUFFER_SIZE)
        {
            FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Log size exceeds maximum expected size! Size = 0x%llx\n",
                                (current - base));

            log_size = AP_ADV_LOGGER_BUFFER_SIZE;
        }
        else
        {
            log_size = sizeof(advanced_logger_info) + (current - base);
        }
    }
    else
    {
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Log corrupted! Current (0x%llx) < Base (0x%llx)!\n", current, base);
    }

    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Log Size = 0x%llx\n", log_size);
    return log_size;
}
