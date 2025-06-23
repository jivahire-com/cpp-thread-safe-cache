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
#include <fuse_artifact_version.h>
#include <fuse_dma.h>    // apply copy fuse to ram
#include <fuse_events.h> // apply event trace for fuse
#include <fuse_init.h>   // fuse service API interface
#include <idhw.h>
// #include <fuse_knobs.h>
#include <fuse_main_i.h>
#include <fuse_struct.h>
#include <fuses_top_regs.h>
#include <icc_platform_defines.h>
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

/*-- Declarations (Statics and globals) --*/

static fpfw_icc_base_ctx_t* icc_base_ctx_fuse;
static fpfw_icc_base_ctx_t* icc_base_ctx_d2dmbx;
kng_fuse_disable_core_t DIE0_fuse_disable, DIE1_fuse_disable;
static kng_fuse_disable_core_t remote_die_core_disable = {0};

static fpfw_icc_base_send_req_t scp_send_params;
static fpfw_icc_base_recv_req_t scp_recv_params;

static rmss_d2d_mailbox_msg d2d_recv_msg;
static rmss_d2d_mailbox_msg d2d_send_msg;

static uint32_t die0_core_disable_config_knob_0_31 = 0;
static uint32_t die0_core_disable_config_knob_32_63 = 0;
static uint32_t die0_core_disable_config_knob_64_95 = 0;

static uint32_t die1_core_disable_config_knob_0_31 = 0;
static uint32_t die1_core_disable_config_knob_32_63 = 0;
static uint32_t die1_core_disable_config_knob_64_95 = 0;
// Enable spare cores
static uint32_t die0_config_spare_core_en_0_31 = 0;
static uint32_t die0_config_spare_core_en_32_63 = 0;
static uint32_t die0_config_spare_core_en_64_95 = 0;

static uint32_t die1_config_spare_core_en_0_31 = 0;
static uint32_t die1_config_spare_core_en_32_63 = 0;
static uint32_t die1_config_spare_core_en_64_95 = 0;

struct ap_core_die_cfg_cb_t
{
    ap_core_die_cfg_cb cb;
    void* context;
};
static struct ap_core_die_cfg_cb_t ap_core_die_cfg_completion = {.cb = NULL, .context = NULL};

/*------------- Functions ----------------*/
void scp_remote_die_config_req_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

// write recieived information via sds
void save_remote_die_config_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    uint32_t result = 0;

    fpfw_icc_base_recv_req_t* req_params = (fpfw_icc_base_recv_req_t*)context;
    if (status != FPFW_STATUS_SUCCESS)
    {
        printf(FUSE_NAME "Failed to receive remote die config request\n");
        return;
    }
    uint32_t* recv_payload = (uint32_t*)req_params->payload_buffer;

    remote_die_core_disable.fuse_dis_core_0_31 = recv_payload[1];
    remote_die_core_disable.fuse_dis_core_32_63 = recv_payload[2];
    remote_die_core_disable.fuse_dis_core_64_95 = recv_payload[3];
    remote_die_core_disable.fuse_dis_core_96_127 = recv_payload[4];

    result = sds_block_creation(FUSE_DISABLE_CORE_DIE1_STRUCT_ID, FUSE_DISABLE_CORE_DIE1_SIZE, PLATFORM_SDS_REGION_ARSM_DIE0);
    BUG_ASSERT(result == KNG_SUCCESS);

    result = sds_block_write(FUSE_DISABLE_CORE_DIE1_STRUCT_ID, &remote_die_core_disable, FUSE_DISABLE_CORE_DIE1_SIZE);
    BUG_ASSERT(result == KNG_SUCCESS);

    // Call the registered callback to notify the completion of remote die config
    if ((NULL != ap_core_die_cfg_completion.cb) && (NULL != ap_core_die_cfg_completion.context))
    {
        ap_core_die_cfg_completion.cb(ap_core_die_cfg_completion.context);
    }
}

int prepare_remote_die_config_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    // prepare recv request
    memset(&scp_recv_params, 0, sizeof(scp_recv_params));

    scp_recv_params.payload_buffer = &d2d_recv_msg;
    scp_recv_params.buffer_size = sizeof(d2d_recv_msg);
    scp_recv_params.recv_cmd_code = RMSS_D2D_MAILBOX_DIE_CONFIG_REQ;
    scp_recv_params.cb = save_remote_die_config_cb;
    scp_recv_params.cb_ctx = &scp_recv_params;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &scp_recv_params);
    BUG_ASSERT(status == KNG_SUCCESS);
    return status;
}

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
    case PLATFORM_EMU:
    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_2D:
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D_8C:
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

int read_core_disables()
{
    kng_fuse_disable_core_t* p_fuse_disable;

    uint32_t status = 0;
    int p_die_num;

    if (idsw_get_die_id() == DIE_0)
    {
        p_fuse_disable = &DIE0_fuse_disable;
        p_die_num = 0;

        // Read Core fuses for die0
        status = read_core_defect_fuses(&(p_fuse_disable->fuse_dis_core_64_95),
                                        &(p_fuse_disable->fuse_dis_core_32_63),
                                        &(p_fuse_disable->fuse_dis_core_0_31));
        if (status != SILIBS_SUCCESS)
        {
            printf(FUSE_NAME "Unable to read cores fuses for DIE%d\n", p_die_num);
            return status;
        }

        // Apply Core disable knobs
        p_fuse_disable->fuse_dis_core_0_31 |= die0_core_disable_config_knob_0_31;
        p_fuse_disable->fuse_dis_core_32_63 |= die0_core_disable_config_knob_32_63;
        p_fuse_disable->fuse_dis_core_64_95 |= (die0_core_disable_config_knob_64_95 | 0xFFFFFFF0);
        p_fuse_disable->fuse_dis_core_96_127 = 0xFFFFFFFF;

        // Apply Spare Enable knobs
        p_fuse_disable->fuse_dis_core_0_31 &= ~die0_config_spare_core_en_0_31;
        p_fuse_disable->fuse_dis_core_32_63 &= ~die0_config_spare_core_en_32_63;
        p_fuse_disable->fuse_dis_core_64_95 &=
            ((p_fuse_disable->fuse_dis_core_64_95 & ~die0_config_spare_core_en_64_95) | 0xFFFFFFF0);
        p_fuse_disable->fuse_dis_core_96_127 = 0xFFFFFFFF;

        printf(FUSE_NAME "Save Core Disable fuse and knob info for DIE%d done\n", p_die_num);
    }
    else
    {
        p_fuse_disable = &DIE1_fuse_disable;
        p_die_num = 1;

        // Read Core fuses for die1
        status = read_core_defect_fuses(&(p_fuse_disable->fuse_dis_core_64_95),
                                        &(p_fuse_disable->fuse_dis_core_32_63),
                                        &(p_fuse_disable->fuse_dis_core_0_31));
        if (status != SILIBS_SUCCESS)
        {
            printf(FUSE_NAME "Unable to read cores fuses for DIE%d\n", p_die_num);
            return status;
        }

        // Apply Core disable knobs
        p_fuse_disable->fuse_dis_core_0_31 |= die1_core_disable_config_knob_0_31;
        p_fuse_disable->fuse_dis_core_32_63 |= die1_core_disable_config_knob_32_63;
        p_fuse_disable->fuse_dis_core_64_95 |= (die1_core_disable_config_knob_64_95 | 0xFFFFFFF0);
        p_fuse_disable->fuse_dis_core_96_127 = 0xFFFFFFFF;

        // Apply Spare Enable knobs
        p_fuse_disable->fuse_dis_core_0_31 &= ~die1_config_spare_core_en_0_31;
        p_fuse_disable->fuse_dis_core_32_63 &= ~die1_config_spare_core_en_32_63;
        p_fuse_disable->fuse_dis_core_64_95 &=
            ((p_fuse_disable->fuse_dis_core_64_95 & ~die1_config_spare_core_en_64_95) | 0xFFFFFFF0);
        p_fuse_disable->fuse_dis_core_96_127 = 0xFFFFFFFF;

        printf(FUSE_NAME "Save Core Disable fuse and knob info for DIE%d done\n", p_die_num);
    }

    return SILIBS_SUCCESS;
}

void register_remote_die_cfg_completion_cb(ap_core_die_cfg_cb cb, void* ctx)
{
    if ((cb == NULL) || (ctx == NULL))
    {
        printf(FUSE_NAME "AP core die cfg cb null\n");
        return;
    }
    // Register the callback to be called later after die 0 recvs fuse info from die 1
    ap_core_die_cfg_completion.cb = cb;
    ap_core_die_cfg_completion.context = ctx;
}

int write_fuse_info_to_ap()
{
    int32_t result = 0;
    // TODO: TASK 2598729 : [SCP] DIE1 sends fuse info to DIE0 to write.This assumes DIE0 and DIE1 has same fuses
    if (idsw_get_die_id() == DIE_0)
    {
        result = sds_block_creation(FUSE_DISABLE_CORE_DIE0_STRUCT_ID, FUSE_DISABLE_CORE_DIE0_SIZE, PLATFORM_SDS_REGION_ARSM_DIE0);
        BUG_ASSERT(result == KNG_SUCCESS);

        result = sds_block_write(FUSE_DISABLE_CORE_DIE0_STRUCT_ID, &DIE0_fuse_disable, FUSE_DISABLE_CORE_DIE0_SIZE);
        BUG_ASSERT(result == KNG_SUCCESS);

        if (idhw_is_single_die_boot_en())
        {
            //! complete the ap core handover immediately
            ap_core_die_cfg_completion.cb(ap_core_die_cfg_completion.context);
        }
    }
    else
    {
        d2d_send_msg.as_uint32[0] = SET_RMSS_D2D_MAILBOX_HEADER_ASUNIT32(RMSS_D2D_MAILBOX_DIE_CONFIG_REQ, 0, 0);
        // set d2d_send_msg_.as_uint32[1] to asuint32_t[3] same as DIE1_fuse_disable[0] to [3]
        d2d_send_msg.as_uint32[1] = DIE1_fuse_disable.fuse_dis_core_0_31;
        d2d_send_msg.as_uint32[2] = DIE1_fuse_disable.fuse_dis_core_32_63;
        d2d_send_msg.as_uint32[3] = DIE1_fuse_disable.fuse_dis_core_64_95;
        d2d_send_msg.as_uint32[4] = DIE1_fuse_disable.fuse_dis_core_96_127;

        scp_send_params.payload_buffer = &d2d_send_msg;
        scp_send_params.buffer_size = sizeof(d2d_send_msg);
        scp_send_params.cb = scp_remote_die_config_req_cb;
        scp_send_params.cb_ctx = &scp_send_params;

        fpfw_status_t icc_status = fpfw_icc_base_send(icc_base_ctx_d2dmbx, &scp_send_params);
        if (icc_status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            printf(FUSE_NAME "send fuse info to primary die fail\n");
            BUG_ASSERT(result == KNG_SUCCESS);
        }

        if ((ap_core_die_cfg_completion.cb == NULL) || (ap_core_die_cfg_completion.context == NULL))
        {
            printf(FUSE_NAME "AP core die cfg cb null\n");
            return FPFW_INIT_STATUS_E_UNKNOWN_ID;
        }
        //! Die 1 can complete the ap core handover immediately
        ap_core_die_cfg_completion.cb(ap_core_die_cfg_completion.context);
    }
    printf(FUSE_NAME "DIE fuse info to ap successfully!\n");
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
            status = read_core_disables();
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
    case PLATFORM_EMU:
    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_2D:
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D_8C:
        printf(FUSE_NAME "Platform is EMU\n");
        status = fuse_dist_get_exclusion_list(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count);
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

void fuse_init(fpfw_icc_base_ctx_t* icc_hspmbx_ctx, fpfw_icc_base_ctx_t* icc_d2dmbx_ctx)
{
    icc_base_ctx_fuse = icc_hspmbx_ctx;
    icc_base_ctx_d2dmbx = icc_d2dmbx_ctx;

    if (!idhw_is_single_die_boot_en() && (idsw_get_die_id() == DIE_0))
    {
        // prepare the listener for remote die config request
        prepare_remote_die_config_listener(icc_base_ctx_d2dmbx);
    }

    if (idsw_get_die_id() == DIE_0)
    {
        die0_core_disable_config_knob_0_31 = config_get_die0_core_disable_value_0_31();
        die0_core_disable_config_knob_32_63 = config_get_die0_core_disable_value_32_63();
        die0_core_disable_config_knob_64_95 = config_get_die0_core_disable_value_64_95();

        die0_config_spare_core_en_0_31 = config_get_die0_core_spare_en_0_31();
        die0_config_spare_core_en_32_63 = config_get_die0_core_spare_en_32_63();
        die0_config_spare_core_en_64_95 = config_get_die0_core_spare_en_64_95();
    }
    else
    {
        die1_core_disable_config_knob_0_31 = config_get_die1_core_disable_value_0_31();
        die1_core_disable_config_knob_32_63 = config_get_die1_core_disable_value_32_63();
        die1_core_disable_config_knob_64_95 = config_get_die1_core_disable_value_64_95();
        // core‐spare‐enable masks
        die1_config_spare_core_en_0_31 = config_get_die1_core_spare_en_0_31();
        die1_config_spare_core_en_32_63 = config_get_die1_core_spare_en_32_63();
        die1_config_spare_core_en_64_95 = config_get_die1_core_spare_en_64_95();
    }
    FUSE_ET_STATUS(FUSE_ET_TYPE_SVC_START);
}

void fuse_print_version(void)
{
    FPFW_DBGPRINT_INFO(FUSE_NAME "Current fuse artifacts version %d.%d.%d \n",
                       ((FUSE_ARTIFACT_VERSION >> 24) & 0xff),
                       ((FUSE_ARTIFACT_VERSION >> 12) & 0xfff),
                       (FUSE_ARTIFACT_VERSION & 0xfff));
}