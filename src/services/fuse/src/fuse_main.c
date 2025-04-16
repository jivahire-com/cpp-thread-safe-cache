/*
 * Copyright (c) Microsoft Corporation.
 *
 * Description:
 *     fuse_main
 *     main support for fuse module API
 */

/*------------- Includes -----------------*/

#include <bug_check.h> // for BUG_ASSERT_PARAM, BUG_ASSERT
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h>   // for fpfw_init_get_handle, FPFW_INIT_C...
#include <fpfw_status.h> // for fpfw_status_t
#include <fuse.h>        // fuse library functions
#include <fuse_dma.h>    // apply copy fuse to ram
#include <fuse_events.h> // apply event trace for fuse
#include <fuse_init.h>   // fuse service API interface
// #include <fuse_knobs.h>
#include <fuse_struct.h>
#include <fuses_top_regs.h>
#include <hsp_firmware_headers.h>
#include <idsw.h> // SW platform id
#include <idsw_kng.h>
#include <kingsgate_fuse_defines.h> // Test revision get
#include <kng_soc_constants.h>      // for DIE_INSTANCE
#include <memory_map/mscp_exp_rmss_memory_map.h>
#include <sds_api.h>
#include <sds_configuration.h>
#include <sds_init.h>
#include <shared_sds_def.h>      //Fuse SDS block and struct id
#include <silibs_mcp_top_regs.h> // for MCP_TOP_MCP2HSP_MAILBOX_ADDRESS
#include <silibs_platform.h>     // silibs status and result
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h> // for SCP_TOP_SCP2HSP_MAILBOX_ADDRESS
#include <stdbool.h>             // for false, true
#include <stdint.h>              // for uintptr_t
#include <stdio.h>               // for NULL, printf
#include <system_info.h>
#include <tx_api.h> // for TX_WAIT_FOREVER, ULONG, tx_queue_receive
#include <utils.h>  // for sleep_ms()...
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
#define NUM_CSR_BACKED_CORE_FUSE_DESCRIPTORS (4)
#define MAX_BYTES_PER_FUSE                   8
#define MAX_BITS_PER_FUSE                    (MAX_BYTES_PER_FUSE * 8)
#define BITS_PER_BYTE                        (8)

/*-------- Function Prototypes -----------*/
static bool platform_requires_fuse_distribution();
static int platform_fuse_copy_to_ram();
static int read_override_from_spi();
static int read_core_disabled_fuses();

/*-- Declarations (Statics and globals) --*/

static fpfw_icc_base_ctx_t* icc_base_ctx_fuse;
kng_fuse_disable_core_t DIE0_fuse_disable, DIE1_fuse_disable;
static uint32_t config_knob_0_31 = 0;
static uint32_t config_knob_32_63 = 0;
static uint32_t config_knob_64_95 = 0;

/*------------- Functions ----------------*/

static bool platform_requires_fuse_distribution()
{
    bool status = false;
    KNG_PLAT_ID plat_id = idsw_get_platform_sdv();
    switch (plat_id)
    {
    case PLATFORM_RVP_EVT_SILICON:
        status = true;
        break;
    // TODO: leave FPGA platforms here until tested working
    // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2002810
    case PLATFORM_RTL_SIM:
        status = true;
        break;
    case PLATFORM_FPGA:
        status = true;
        break;
    case PLATFORM_FPGA_LARGE:
        status = true;
        break;
    case PLATFORM_FPGA_LARGE_RVP:
        status = true;
        break;
    case PLATFORM_SVP_SIM:
        status = true;
        break;
    default:
        printf(FUSE_NAME "Fuse distribution not supported in FW for platform\n");
        break;
    }
    return status;
}

static int platform_fuse_copy_to_ram()
{
    return fuse_dma_copy_to_ram_blocking();
}

static int read_override_from_spi()
{
    fpfw_status_t icc_status = 0;

    size_t recv_msg_size_bytes = 0;

    kng_hsp_fuse_mailbox_msg fuse_fw_load = {};
    fuse_fw_load.fuse_req.header.cmd = HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_REQ;
    if (idsw_get_die_id() == DIE_0)
    {
        fuse_fw_load.fuse_req.id = HSP_FIRMWARE_ID_FUSE_OVERRIDE_DIE_0;
    }
    else
    {
        fuse_fw_load.fuse_req.id = HSP_FIRMWARE_ID_FUSE_OVERRIDE_DIE_1;
    }
    fuse_fw_load.fuse_req.address = SCP_EXP_FUSE_DATA_BASE;
    fuse_fw_load.fuse_req.size = SCP_EXP_FUSE_DATA_SIZE;
    icc_status =
        fpfw_icc_base_send_recv_sync(icc_base_ctx_fuse, &fuse_fw_load, sizeof(kng_hsp_fuse_mailbox_msg), &recv_msg_size_bytes);

    return icc_status;
}

static int read_core_disabled_fuses()
{
    kng_fuse_disable_core_t* p_fuse_disable;

    uint32_t status = 0;
    int p_die_num;

    if (idsw_get_die_id() == DIE_0)
    {
        p_fuse_disable = &DIE0_fuse_disable;
        p_die_num = 0;
    }
    else
    {
        p_fuse_disable = &DIE1_fuse_disable;
        p_die_num = 1;
    }

    status = read_core_defect_fuses(&(p_fuse_disable->fuse_dis_core_64_95),
                                    &(p_fuse_disable->fuse_dis_core_32_63),
                                    &(p_fuse_disable->fuse_dis_core_0_31));
    if (status != SILIBS_SUCCESS)
    {
        printf(FUSE_NAME "get the defect knob fail\n");
        return status;
    }
    printf(FUSE_NAME "save disable knob in DIE%d done\n", p_die_num);

    return SILIBS_SUCCESS;
}

// USE SDS API to write core fuse info to AP
int write_fuse_info_to_ap()
{
    int32_t result = 0;
    if (idsw_get_die_id() == DIE_0)
    {
        result = sds_block_creation(FUSE_DISABLE_CORE_DIE0_STRUCT_ID, FUSE_DISABLE_CORE_DIE0_SIZE, PLATFORM_SDS_REGION_ARSM_DIE0);
        BUG_ASSERT(result == KNG_SUCCESS);

        DIE0_fuse_disable.fuse_dis_core_0_31 |= config_knob_0_31;
        DIE0_fuse_disable.fuse_dis_core_32_63 |= config_knob_32_63;
        DIE0_fuse_disable.fuse_dis_core_64_95 |= (config_knob_64_95 | 0xFFFFFFF0);
        DIE0_fuse_disable.fuse_dis_core_96_127 = 0xFFFFFFFF;
        result = sds_block_write(FUSE_DISABLE_CORE_DIE0_STRUCT_ID, &DIE0_fuse_disable, FUSE_DISABLE_CORE_DIE0_SIZE);
        BUG_ASSERT(result == KNG_SUCCESS);
    }
    else
    {
        result = sds_block_creation(FUSE_DISABLE_CORE_DIE1_STRUCT_ID, FUSE_DISABLE_CORE_DIE1_SIZE, PLATFORM_SDS_REGION_ARSM_DIE0);
        BUG_ASSERT(result == KNG_SUCCESS);
        DIE1_fuse_disable.fuse_dis_core_0_31 |= config_knob_0_31;
        DIE1_fuse_disable.fuse_dis_core_32_63 |= config_knob_32_63;
        DIE1_fuse_disable.fuse_dis_core_64_95 |= (config_knob_64_95 | 0xFFFFFFF0);
        DIE1_fuse_disable.fuse_dis_core_96_127 = 0xFFFFFFFF;
        result = sds_block_write(FUSE_DISABLE_CORE_DIE1_STRUCT_ID, &DIE1_fuse_disable, FUSE_DISABLE_CORE_DIE1_SIZE);
        BUG_ASSERT(result == KNG_SUCCESS);
    }
    printf(FUSE_NAME "DIE%d fuse info to ap successfully!\n", idsw_get_die_id());
    return result;
}

int platform_read_for_fuse(const uintptr_t fuse_store_addr, const uint64_t fuse_bit_offset, const uint32_t fuse_bit_size)
{
    uint64_t fuse_data = 0;
    if ((fuse_bit_size == 0) || (fuse_bit_size > MAX_BITS_PER_FUSE))
    {
        printf(FUSE_NAME "Requested Fuse Size in bits not valid(Min:%d Max:%d bits) \n", 1, MAX_BITS_PER_FUSE);
        return FPFW_INIT_STATUS_E_UNKNOWN_ID;
    }
    fuse_data = fuse_read(fuse_bit_offset, fuse_bit_size);
    // number of valid bytes to copy from fuse_data
    size_t fuse_size = ((fuse_bit_size + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
    memcpy((void*)fuse_store_addr, (void*)&fuse_data, fuse_size);

    // TODO: check if the incoming request is before overrides have been applied.
    // ADO 948518
    // status = FPFW_INIT_STATUS_SUCCESS;
    return FPFW_INIT_STATUS_SUCCESS;
}

int platform_fuse_override()
{
    int status = 0;
    uint32_t Kingsgate_fuse_override_buffer_location = (uint32_t)(SCP_EXP_FUSE_DATA_BASE);
    DIE_INSTANCE die_id = (DIE_INSTANCE)idsw_get_die_id();
    KNG_PLAT_ID plat_id = idsw_get_platform_sdv();

    // Skip for platforms that do not support fuses
    if (platform_requires_fuse_distribution())
    {
        // Cache base fuse data from efuse blocks into SoC RAM location
        status = platform_fuse_copy_to_ram();
        FUSE_ET_STATUS(FUSE_ET_TYPE_RAM_DMA_COPY);
        printf(FUSE_NAME "copy_to_ram status=%d\n", status);
        BUG_ASSERT_PARAM((status == SILIBS_SUCCESS), status, SILIBS_SUCCESS);

        /* Enable fuse feature once DMA fuse copy is successful */
        fuse_feature_enable(true);

        if (!config_get_fuse_knobs().fuse_ignore_ifwi_overrides)
        {
            const bool is_fused_part =
                (fuse_read(SILICON_ID_SILICON_MAJOR_REVISION_BIT_OFFSET, SILICON_ID_SILICON_MAJOR_REVISION_WIDTH) != 0);
            printf(FUSE_NAME "if fused part [%d] \n", is_fused_part);
            // wait for the HSP Fuse for HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_REQ so I will only pass for SVP
            if (plat_id == PLATFORM_RVP_EVT_SILICON)
            {
                status = read_override_from_spi(); // Read override buffer from SPI Flash and apply to fuses if present
            }
            else
            {
                printf(FUSE_NAME "Non_support_machine!\n");
                status = SILIBS_E_SUPPORT;
            }

            FUSE_ET_STATUS(FUSE_ET_TYPE_MAILBOX_REQUEST_OVERRIDES);
            const bool fuse_overrides_present = (status == SILIBS_SUCCESS);
            if (!is_fused_part && !fuse_overrides_present)
            {
                FUSE_ET_STATUS(FUSE_ET_TYPE_FUSED_NO_OVERRIDES);
                printf(FUSE_NAME "fuse no override\n");
                if (status != SILIBS_SUCCESS)
                {
                    printf(FUSE_NAME "Fuse no override!\n");
                    status = FUSE_NO_OVERRIDES;
                    return status;
                }
            }
            else if (!is_fused_part && fuse_overrides_present)
            {
                printf(FUSE_NAME "Unfused part with fuse overrides in SPI. Applying overrides ignoring "
                                 "per-fuse valids.\n");
                status = fuse_override_ignoring_valids(die_id, Kingsgate_fuse_override_buffer_location);
                FUSE_ET_STATUS(FUSE_ET_TYPE_FUSED_IGNORE_VALIDS);
                if (status != SILIBS_SUCCESS)
                {
                    printf(FUSE_NAME "fuse_override_ignoring_valids fail!\n");
                    status = FUSE_ERROR_IGNORE_VALIDS;
                    return status;
                }
            }
            else if (is_fused_part && !fuse_overrides_present)
            {
                printf(FUSE_NAME "Fused part with no fuse overrides in SPI.\n");
            }
            else
            {
                printf(FUSE_NAME "Fused part with fuse overrides in SPI. Applying all valid overrides.\n");
                status = fuse_override(die_id, Kingsgate_fuse_override_buffer_location);
                FUSE_ET_STATUS(FUSE_ET_TYPE_FUSED_WITH_OVERRIDES);

                if (status != SILIBS_SUCCESS)
                {
                    printf(FUSE_NAME "fuse_override fail!\n");
                    status = FUSE_ERROR_WITH_OVERRIDES;
                    return status;
                }
            }
        }
        if (!system_info_is_warm_start())
        {
            status = read_core_disabled_fuses();
            if (status != SILIBS_SUCCESS)
            {
                printf(FUSE_NAME "save disable knob failed\n");
                status = FUSE_ERROR_DISABLE_KNOB;
                return status;
            }
        }

        trigger_debugger_for_manual_overrides();
    }
    FUSE_ET_STATUS(FUSE_ET_TYPE_OVERRIDE_COMPLETE);
    printf(FUSE_NAME "Fuse Override complete\n");
    return status;
}

int platform_fuse_distribution(fuse_distribution_stage_t stage)
{
    int status = 0;
    const fuse_dist_exclude_range_t* fuse_dist_exclude_list = NULL;
    uint32_t exclude_list_count = 0;
    KNG_PLAT_ID plat_id = idsw_get_platform_sdv();
    DIE_INSTANCE die_id = (DIE_INSTANCE)idsw_get_die_id();

    switch (plat_id)
    {
    case PLATFORM_FPGA:
        printf(FUSE_NAME " Platform is FPGA\n");
        status = fuse_dist_get_exclusion_list(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count);
        break;
    case PLATFORM_FPGA_LARGE:
        printf(FUSE_NAME "Platform is FPGA_Large\n");
        status = fuse_dist_get_exclusion_list(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count);
        break;
    case PLATFORM_FPGA_LARGE_RVP:
        printf(FUSE_NAME "Platform is FPGA_Large_RVP\n");
        status = fuse_dist_get_exclusion_list(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count);
        break;
    case PLATFORM_RVP_EVT_SILICON:
        printf(FUSE_NAME "Platform is PLATFORM_RVP_EVT_SILICON\n");
        break;
    case PLATFORM_SVP_SIM:
        printf(FUSE_NAME "Platform is PLATFORM_SVP_SIM\n");
        status = fuse_dist_get_exclusion_list(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count);
        break;
    default:
        status = SILIBS_E_SUPPORT;
        return status;
    }

    if (status != SILIBS_SUCCESS)
    {
        printf(FUSE_NAME " GET_EXCLUSION_LIST Fail\n");
        status = FUSE_ERROR_GET_EXCLUSION_LIST;
        return status;
    }

    printf(FUSE_NAME "Fuse Distribution Start\n");
    FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_START);
    if (system_info_get_security_state() == HSP_SECURITY_STATE_SECURE)
    {
        printf(FUSE_NAME "Fuse Distribution is not supported in secure state\n");
        return FUSE_ERROR_NO_DISTRIBUTION;
    }

    if (platform_requires_fuse_distribution())
    {
        if (stage == FUSE_DISTRIBUTION_STAGE_POST_HSP)
        {
            status = fuse_distribution(die_id, POST_HSP_DIST_MAJOR, POST_HSP_DIST_MINOR, fuse_dist_exclude_list, exclude_list_count);
            FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR3_MINOR0);
            if (status != SILIBS_SUCCESS)
            {
                printf(FUSE_NAME "DISTRIBUTION_PHASE_MAJOR3_MINOR0 Fail\n");
                status = FUSE_ERROR_DISTRIBUTION_PHASE_MAJOR3_MINOR0;
                return status;
            }
        }
        else if (stage == FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT)
        {
            status = fuse_distribution(die_id, POST_HSP_DIST_MAJOR, MESH_INIT_MINOR, fuse_dist_exclude_list, exclude_list_count);
            FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR3_MINOR1);
            if (status != SILIBS_SUCCESS)
            {
                printf(FUSE_NAME "DISTRIBUTION_PHASE_MAJOR3_MINOR1 Fail\n");
                status = FUSE_ERROR_DISTRIBUTION_PHASE_MAJOR3_MINOR1;
                return status;
            }
        }
        else if (stage == FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT)
        {
            status = fuse_distribution(die_id, POST_MESH_INIT_MAJOR, POST_MESH_INIT_MINOR, fuse_dist_exclude_list, exclude_list_count);
            FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR4_MINOR0);
            if (status != SILIBS_SUCCESS)
            {
                printf(FUSE_NAME "DISTRIBUTION_PHASE_MAJOR4_MINOR0 Fail\n");
                status = FUSE_ERROR_DISTRIBUTION_PHASE_MAJOR4_MINOR0;
                return status;
            }
        }
        else if (stage == FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT)
        {
            status = fuse_distribution(die_id, POST_MESH_INIT_MAJOR, POST_BRIDGE_INIT_MINOR, fuse_dist_exclude_list, exclude_list_count);
            FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR4_MINOR1);
            if (status != SILIBS_SUCCESS)
            {
                status = FUSE_ERROR_DISTRIBUTION_PHASE_MAJOR4_MINOR1;
                printf(FUSE_NAME "DISTRIBUTION_PHASE_MAJOR4_MINOR1 Fail\n");
                return status;
            }
            printf(FUSE_NAME "Phase 3 fuse distribution complete\n");
        }

        printf(FUSE_NAME "Phase [%d] fuse distribution complete\n", stage);
        FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_END);
    }
    else
    {
        printf(FUSE_NAME "Platform does not support fuse distribution\n");
        return FUSE_ERROR_NO_DISTRIBUTION;
    }

    return status;
}

void fuse_init(fpfw_icc_base_ctx_t* icc_base_ctx)
{
    icc_base_ctx_fuse = icc_base_ctx;
    config_knob_0_31 = config_get_core_disable_value_0_31();
    config_knob_32_63 = config_get_core_disable_value_32_63();
    config_knob_64_95 = config_get_core_disable_value_64_95();

    FUSE_ET_STATUS(FUSE_ET_TYPE_SVC_START);
}
