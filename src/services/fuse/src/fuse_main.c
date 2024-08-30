/*
 * Copyright (c) Microsoft Corporation.
 *
 * Description:
 *     fuse_main
 *     main support for fuse module API
 */

/*------------- Includes -----------------*/

#include <bug_check.h>     // for BUG_ASSERT_PARAM, BUG_ASSERT
#include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <fpfw_init.h>     // for fpfw_init_get_handle, FPFW_INIT_C...
#include <fpfw_status.h>   // for fpfw_status_t
#include <fuse.h>          // fuse library functions
#include <fuse_dma.h>      // apply copy fuse to ram
#include <fuse_events.h>   // apply event trace for fuse
#include <fuse_init.h>     // fuse service API interface
#include <fuse_struct.h>
#include <fuses_top_regs.h>
#include <hsp_firmware_headers.h>
#include <idsw.h> // SW platform id
#include <idsw_kng.h>
#include <kingsgate_fuse_defines.h> // Test revision get
#include <kng_soc_constants.h>      // for DIE_INSTANCE
#include <silibs_mcp_top_regs.h>    // for MCP_TOP_MCP2HSP_MAILBOX_ADDRESS
#include <silibs_platform.h>        // silibs status and result
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h> // for SCP_TOP_SCP2HSP_MAILBOX_ADDRESS
#include <stdbool.h>             // for false, true
#include <stdint.h>              // for uintptr_t
#include <stdio.h>               // for NULL, printf
#include <utils.h>               // for sleep_ms()...
/*-- Symbolic Constant Macros (defines) --*/
/*------------- Typedefs -----------------*/
#define NUM_CSR_BACKED_CORE_FUSE_DESCRIPTORS (4)
#define MAX_BYTES_PER_FUSE                   8
#define MAX_BITS_PER_FUSE                    (MAX_BYTES_PER_FUSE * 8)
#define BITS_PER_BYTE                        (8)
#define FUSE_NAME                            "[KNG Fuse] "
/*-------- Function Prototypes -----------*/
static bool platform_requires_fuse_distribution();
static int platform_fuse_copy_to_ram();
static int read_override_from_spi();
static void fuse_icc_base_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status);
static void request_load_fuse_send_complete_cb(void* context, fpfw_status_t status);
/*-- Declarations (Statics and globals) --*/

static kng_hsp_mailbox_msg recv_payload_buffer;
static fpfw_icc_base_ctx_t* icc_base_ctx_fuse;

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
    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
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
static void request_load_fuse_send_complete_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

static void fuse_icc_base_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);
    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT(recv_payload_buffer.header.cmd == HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_RSP);
    BUG_ASSERT(recv_payload_buffer.rsp.status == 0);
    printf(FUSE_NAME "Request FW load received \n");
    FUSE_ET_STATUS(FUSE_ET_TYPE_MAILBOX_REQUEST_OVERRIDES);
}

static int read_override_from_spi()
{
    static fpfw_icc_base_recv_req_t recv_params = {
        .payload_buffer = &recv_payload_buffer,
        .buffer_size = sizeof(recv_payload_buffer),
        .recv_cmd_code = HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_RSP,
        .cb = fuse_icc_base_recv_complete_notify,
        .cb_ctx = NULL,
    };
    fpfw_status_t status = fpfw_icc_base_recv(icc_base_ctx_fuse, &recv_params);
    BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);
    static kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_REQ,
        .header.context = 0,
        .id = HSP_FIRMWARE_ID_FUSE_OVERRIDE_DIE_0,
        // Todo The HSP need to decide the address is
        // https://azurecsi.visualstudio.com/Dev/_workitems/edit/1937493
        .address = SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS,
        .size = SCP_EXP_TOP_RAM1_SIZE,
    };
    static fpfw_icc_base_send_req_t send_params = {
        .payload_buffer = &send_request,
        .cb = request_load_fuse_send_complete_cb,
        .cb_ctx = NULL,
        .buffer_size = sizeof(send_request),
    };
    status = fpfw_icc_base_send(icc_base_ctx_fuse, &send_params);
    BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);
    return status;
}
int platform_read_for_fuse(const uintptr_t fuse_store_addr, const uint64_t fuse_bit_offset, const uint32_t fuse_bit_size)
{
    uint64_t fuse_data = 0;
    if ((fuse_bit_size == 0) || (fuse_bit_size > MAX_BITS_PER_FUSE))
    {
        printf(FUSE_NAME "Requested Fuse Size in bits not valid(Min:%d Max:%d bits) \n", 1, MAX_BITS_PER_FUSE);
        return FPFW_INIT_STATUS_E_UNKNOWN_ID;
    }
    fuse_data = read_fuse(fuse_bit_offset, fuse_bit_size);
    // number of valid bytes to copy from fuse_data
    size_t fuse_size = ((fuse_bit_size + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
    memcpy((void*)fuse_store_addr, (void*)&fuse_data, fuse_size);
    // printf("[ingGate_FUSE] Success: Requested Fuse of byte size:%zu copied to 0x%llX \n", fuse_size,
    // fuse_store_addr); Always return Success.
    // TODO: check if the incoming request is before overrides have been applied.
    // ADO 948518
    // status = FPFW_INIT_STATUS_SUCCESS;
    return FPFW_INIT_STATUS_SUCCESS;
}
int platform_fuse_override()
{
    int status = 0;
    uint32_t Kingsgate_fuse_override_buffer_location = (uint32_t)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS);
    DIE_INSTANCE die_id = (DIE_INSTANCE)idsw_get_die_id();

    // Skip for platforms that do not support fuses
    if (platform_requires_fuse_distribution())
    {
        // Cache base fuse data from efuse blocks into SoC RAM location
        status = platform_fuse_copy_to_ram();
        FUSE_ET_STATUS(FUSE_ET_TYPE_RAM_DMA_COPY);
        BUG_ASSERT_PARAM((status == SILIBS_SUCCESS), status, SILIBS_SUCCESS);

        const bool is_fused_part =
            (read_fuse(SILICON_ID_SILICON_MAJOR_REVISION_BIT_OFFSET, SILICON_ID_SILICON_MAJOR_REVISION_WIDTH) != 0);
        status = read_override_from_spi(); // Read override buffer from SPI Flash and apply to fuses if present

        FUSE_ET_STATUS(FUSE_ET_TYPE_MAILBOX_REQUEST_OVERRIDES);
        const bool fuse_overrides_present = (status == SILIBS_SUCCESS);
        if (!is_fused_part && !fuse_overrides_present)
        {
            FUSE_ET_STATUS(FUSE_ET_TYPE_FUSED_NO_OVERRIDES);
            BUG_ASSERT(false);
        }
        else if (!is_fused_part && fuse_overrides_present)
        {
            printf(FUSE_NAME "Unfused part with fuse overrides in SPI. Applying overrides ignoring "
                             "per-fuse valids.\n");
            status = apply_fuse_override_ignoring_valids(die_id, Kingsgate_fuse_override_buffer_location);
            FUSE_ET_STATUS(FUSE_ET_TYPE_FUSED_IGNORE_VALIDS);
            if (status != SILIBS_SUCCESS)
            {
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
            status = apply_fuse_override(die_id, Kingsgate_fuse_override_buffer_location);
            FUSE_ET_STATUS(FUSE_ET_TYPE_FUSED_WITH_OVERRIDES);
            if (status != SILIBS_SUCCESS)
            {
                return status;
            }
        }
        // Provide an opportunity for manual fuse override - debugger platforms should set a breakpoint on the
        // access implemented by this call
        trigger_debugger_for_manual_overrides();
    }
    FUSE_ET_STATUS(FUSE_ET_TYPE_OVERRIDE_COMPLETE);
    return status;
}
int platform_fuse_distribution(int stage)
{
    int status = 0;
    const fuse_dist_exclude_range_t* fuse_dist_exclude_list = NULL;
    uint32_t exclude_list_count = 0;
    KNG_PLAT_ID plat_id = idsw_get_platform_sdv();
    DIE_INSTANCE die_id = (DIE_INSTANCE)idsw_get_die_id();

    switch (plat_id)
    {
    case PLATFORM_FPGA:
        printf("[KingGate_FUSE] Platform is FPGA\n");
        fuse_dist_get_exclusion_list(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count);
        break;
    case PLATFORM_FPGA_LARGE:
        printf(FUSE_NAME "Platform is FPGA_Large\n");
        fuse_dist_get_exclusion_list(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count);
        break;
    case PLATFORM_FPGA_LARGE_RVP:
        printf(FUSE_NAME "Platform is FPGA_Large_RVP\n");
        fuse_dist_get_exclusion_list(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count);
        break;
    case PLATFORM_RVP_EVT_SILICON:
        printf(FUSE_NAME "Platform is PLATFORM_RVP_EVT_SILICON\n");
        break;
    default:
        status = SILIBS_E_SUPPORT;
        return status;
    }

    printf(FUSE_NAME "Fuse Distribution Start\n");
    FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_START);
    if (platform_requires_fuse_distribution())
    {
        if (stage == 0)
        {
            status = distribute_fuses(die_id, POST_HSP_DIST_MAJOR, POST_HSP_DIST_MINOR, fuse_dist_exclude_list, exclude_list_count);
            FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR3_MINOR0);
            if (status != SILIBS_SUCCESS)
            {
                return status;
            }
            printf(FUSE_NAME "Phase 0 fuse distribution complete\n");
        }
        else if (stage == 1)
        {
            status = distribute_fuses(die_id, POST_HSP_DIST_MAJOR, MESH_INIT_MINOR, fuse_dist_exclude_list, exclude_list_count);
            FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR3_MINOR1);
            if (status != SILIBS_SUCCESS)
            {
                return status;
            }
            printf(FUSE_NAME "Phase 1 fuse distribution complete\n");

            status = distribute_fuses(die_id, POST_MESH_INIT_MAJOR, POST_MESH_INIT_MINOR, fuse_dist_exclude_list, exclude_list_count);
            FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR4_MINOR0);
            if (status != SILIBS_SUCCESS)
            {
                return status;
            }
            printf(FUSE_NAME "Phase 2 fuse distribution complete\n");

            status = distribute_fuses(die_id, POST_MESH_INIT_MAJOR, POST_BRIDGE_INIT_MINOR, fuse_dist_exclude_list, exclude_list_count);
            FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR4_MINOR1);

            if (status != SILIBS_SUCCESS)
            {
                return status;
            }
            printf(FUSE_NAME "Phase 3 fuse distribution complete\n");

            // TODO Task: Integrate with ICC for HSP mailbox load of override
            // https://azurecsi.visualstudio.com/Dev/_workitems/edit/1884658
            // status = write_fuse_info_to_spi();
        }
        printf(FUSE_NAME "fuse distribution complete \n");
        FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_END);
    }
    return status;
}
// This placeholder here is to verify the Fuse event trace log
void fuse_init(fpfw_icc_base_ctx_t* icc_base_ctx)
{
    icc_base_ctx_fuse = icc_base_ctx;
    FUSE_ET_STATUS(FUSE_ET_TYPE_SVC_START);
}
