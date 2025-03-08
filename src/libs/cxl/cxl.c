//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Publishes the CHBCR (CXL Host Bridge Configuration Region) for CXL memory decoders to DDR.
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <cxl.h>
#include <fpfw_cfg_mgr.h>
#include <FpFwAssert.h>
#include <pcie_config_variable.h>
#include <idsw_kng.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static uint8_t get_rp_port_number_from_cxl_port(CXL_PORT port)
{
    switch (port)
    {
        case CXL_RPSS0_RP0:
            return config_get_pcie_rpss0_rp0_cfg().pcie_rp_port_num;
            break;
        case CXL_RPSS1_RP0:
            return config_get_pcie_rpss1_rp0_cfg().pcie_rp_port_num;
            break;
        case CXL_RPSS2_RP0:
            return config_get_pcie_rpss2_rp0_cfg().pcie_rp_port_num;
            break;
        case CXL_RPSS3_RP0:
            return config_get_pcie_rpss3_rp0_cfg().pcie_rp_port_num;
            break;
        case CXL_RPSS4_RP0:
            return config_get_pcie_rpss4_rp0_cfg().pcie_rp_port_num;
            break;
        case CXL_RPSS5_RP0:
            return config_get_pcie_rpss5_rp0_cfg().pcie_rp_port_num;
            break;
        case CXL_RPSS6_RP0:
            return config_get_pcie_rpss6_rp0_cfg().pcie_rp_port_num;
            break;
        case CXL_RPSS7_RP0:
            return config_get_pcie_rpss7_rp0_cfg().pcie_rp_port_num;
            break;
        default:
            break;
    }

    // Should never reach here
    FPFW_UNREACHABLE();
    return 0xFF;
}

uint64_t set_cxl_mem_region_base_and_size(cxl_hdm_decoder_region_t* hdm_decoder, uint64_t *base, uint8_t num_targets)
{
    FPFW_RUNTIME_ASSERT(hdm_decoder != NULL);
    FPFW_RUNTIME_ASSERT(base != NULL);
    FPFW_RUNTIME_ASSERT(num_targets > 0);
    FPFW_RUNTIME_ASSERT(num_targets <= INTERLEAVE_4_WAYS);

    hdm_decoder->base_low.base_low = (uint32_t)((*base >> 28) & 0xF);           // bits 31:28 of the base address
    hdm_decoder->base_high.base_high = (uint32_t)((*base >> 32) & UINT32_MAX);  // bits 63:32 of the base address

    uint64_t size = ((CXL_MEM_REGION_SIZE) * num_targets);
    hdm_decoder->size_low.size_low = (uint32_t)((size >> 28) & 0xF);            // bits 31:28 of the size
    hdm_decoder->size_high.size_high = (uint32_t)((size >> 32) & UINT32_MAX);   // bits 63:32 of the size

    return size;
} 

void cxl_chbcr_init( uint8_t* chbcr_base_address)
{
    cxl_region_params_t cxl_params[NUM_CXL_PARAMS]  = {
                                                config_get_cxl_params_die0(), 
                                                config_get_cxl_params_die1()
                                                };

    // Create CHBCR header starting at the base address
    uint8_t* target_address = chbcr_base_address;
    cxl_chbcr_header_t* chbcr_header = (cxl_chbcr_header_t*)target_address;
    memset(chbcr_header, 0, sizeof(cxl_chbcr_header_t));

    chbcr_header->capability_reg.target_count = 4; // 4 targets per decoder
    chbcr_header->capability_reg.a11to8_interleave_capable = 1;  // must be set
    chbcr_header->capability_reg.a14to12_interleave_capable = 1; // must be set

    target_address += sizeof(cxl_chbcr_header_t);

    uint8_t decoder_count = 0;
    uint64_t cxl_region_base = CXL_MEM_REGION_BASE_ADDR;
    for (int i = 0; i < NUM_CXL_PARAMS; i++)
    {
        if (cxl_params[i].valid == false)
        {
            continue;
        }

        // If interleaving is enabled, create one HDM decoder entry with multiple targets
        if (cxl_params[i].interleave_ways != INTERLEAVE_NONE)
        {
            cxl_hdm_decoder_region_t* cxl_region = (cxl_hdm_decoder_region_t*)target_address;
            memset(cxl_region, 0, sizeof(cxl_hdm_decoder_region_t));

            cxl_region->control.interleave_granularity = cxl_params[i].interleave_size;
            cxl_region->control.interleave_ways = cxl_params[i].interleave_ways;
            printf("CHBCR: Adding decoder with interleave size %d, ways %d\n", cxl_params[i].interleave_size, cxl_params[i].interleave_ways);

            uint64_t cxl_region_size = set_cxl_mem_region_base_and_size(cxl_region, &cxl_region_base, cxl_params[i].interleave_ways);
            printf("CHBCR: Decoder with base 0x%lX%08lX size 0x%lX%08lX\n", (unsigned long)(cxl_region_base >> 32), (unsigned long)cxl_region_base, (unsigned long)(cxl_region_size >> 32), (unsigned long)(cxl_region_size));
            // Increment region base
            cxl_region_base += cxl_region_size;

            for (int port_index = 0; port_index < (int)cxl_params[i].interleave_ways; port_index++)
            {
                // Port number is defined via knob and set in the link capabilities register
                cxl_region->target_list.target_port_id[port_index] = get_rp_port_number_from_cxl_port(cxl_params[i].ports[port_index]);
                printf("CHBCR: Target Port %d\n", cxl_region->target_list.target_port_id[port_index]);
            }

            decoder_count++;
            target_address += sizeof(cxl_hdm_decoder_region_t);
        }
        else 
        {
            // Create one HDM decoder entry per port
            for (int port_index = 0; (cxl_params[i].ports[port_index] != CXL_PORT_EMPTY) && (port_index < (int)INTERLEAVE_4_WAYS); port_index++)
            {
                cxl_hdm_decoder_region_t* cxl_region = (cxl_hdm_decoder_region_t*)target_address;
                memset(cxl_region, 0, sizeof(cxl_hdm_decoder_region_t));

                cxl_region->control.interleave_ways = cxl_params[i].interleave_ways;
                printf("CHBCR: Adding decoder with no interleave\n");

                uint64_t cxl_region_size = set_cxl_mem_region_base_and_size(cxl_region, &cxl_region_base, 1);
                printf("CHBCR: Decoder with base 0x%lX%08lX size 0x%lX%08lX\n", (unsigned long)(cxl_region_base >> 32), (unsigned long)cxl_region_base, (unsigned long)(cxl_region_size >> 32), (unsigned long)(cxl_region_size));

                // Increment region base
                cxl_region_base += cxl_region_size;

                cxl_region->target_list.target_port_id[0] = get_rp_port_number_from_cxl_port(cxl_params[i].ports[port_index]);
                printf("CHBCR: Target Port %d\n", cxl_region->target_list.target_port_id[0]);

                decoder_count++;
                target_address += sizeof(cxl_hdm_decoder_region_t);
            }
        }
    }

    // num of decoders == n+1
    chbcr_header->capability_reg.decoder_count = decoder_count == 0 ? 0 : decoder_count - 1;
}