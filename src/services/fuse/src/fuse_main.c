/*
 * Copyright (c) Microsoft Corporation.
 *
 * Description:
 *     fuse_main
 *     main support for fuse module API
 */

/*------------- Includes -----------------*/

#include <FpFwAssert.h>              // for FPFW_RUNTIME_ASSERT
#include <fpfw_init.h>               // for fpfw_init_get_handle, FPFW_INIT_C...
#include <fpfw_mbox_icc_transport.h> // for ICC_MBX_ASYNC_RECV, ICC_MBX_ASY...
#include <fpfw_status.h>             // for fpfw_status_t
#include <fuse.h>                    // fuse library functions
#include <fuse_defines.h>            // Test revision get
#include <fuse_dma.h>                // apply copy fuse to ram
#include <fuse_events.h>             // apply event trace for fuse
#include <fuse_init.h>               // fuse service API interface
#include <fuse_struct.h>
#include <fuses_top_regs.h>
#include <idsw.h> // SW platform id
#include <idsw_kng.h>
#include <silibs_mcp_top_regs.h> // for MCP_TOP_MCP2HSP_MAILBOX_ADDRESS
#include <silibs_platform.h>     // silibs status and result
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
/*-- Declarations (Statics and globals) --*/
/*------------- Functions ----------------*/
static bool platform_requires_fuse_distribution()
{
    bool status = false;
    KNG_PLAT_ID plat_id = idsw_get_platform_sdv();
    if ((plat_id == PLATFORM_FPGA) || (plat_id == PLATFORM_RTL_SIM) || (plat_id == PLATFORM_FPGA_LARGE) ||
        (plat_id == PLATFORM_RVP_EVT_SILICON) || (plat_id == PLATFORM_FPGA_LARGE_RVP))
    {
        status = true;
    }
    return status;
}
static int platform_fuse_copy_to_ram()
{
    return fuse_dma_copy_to_ram_blocking();
}
static int read_override_from_spi()
{
    int retval = 0;
// TODO:  Integrate with ICC
// https://azurecsi.visualstudio.com/Dev/_workitems/edit/1884658
#ifdef _HAS_ICC_READY
    static fpfw_mbox_icc_transport_intrf_t mscp_mbx_intf = {};
    HSP_MAILBOX_MSG message;
    HSP_MAILBOX_MSG response;
    size_t output_size = 0;
    fpfw_status_t status = fpfw_mbox_icc_transport_dfwk_interface_init(fpfw_init_get_handle("icc_mbx"), &mscp_mbx_intf);
    FPFW_RUNTIME_ASSERT(status == FPFW_INIT_STATUS_SUCCESS);
    retval = fpfw_icc_transport_get_max_mesg_size_sync_req(mscp_mbx_intf, &output_size);
    FPFW_RUNTIME_ASSERT(retval == FPFW_INIT_STATUS_SUCCESS);
    message.Cmd = HstpMailboxCmdLoadFirmware;
    message.Msg[0] = HspFirmwareIdFuseOverride;
    message.Msg[1] = SCP_TOP_SCP_EXP_ADDRESS;
    message.Msg[2] = SCP_EXP_TOP_RAM0_ADDRESS;
    printf("\n");
    do
    {
        // mailbox send may fail if response is in fifo (w/ additional response also queued); attempt to read/clear any pending response.
        fpfw_icc_transport_try_recv_sync_req(mscp_mbx_intf, &response, sizeof(HSP_MAILBOX_MSG), &output_size);
    } while (fpfw_icc_transport_try_send_sync_req(mscp_mbx_intf, &message) != FPFW_INIT_STATUS_SUCCESS);
    response.Cmd = 0;
    while ((fpfw_icc_transport_try_recv_sync_req(mscp_mbx_intf, &response, sizeof(HSP_MAILBOX_MSG), &output_size) !=
            FPFW_INIT_STATUS_SUCCESS) ||
           (response.Cmd != message.Cmd))
    {
        sleep_ms(1);
    }
#else
    return SILIBS_E_SUPPORT;
#endif
    return retval;
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
    // Skip for platforms that do not support fuses
    if (platform_requires_fuse_distribution())
    {
        // Cache base fuse data from efuse blocks into SoC RAM location
        status = platform_fuse_copy_to_ram();
        FUSE_ET_STATUS(FUSE_ET_TYPE_RAM_DMA_COPY);
        FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);
        const bool is_fused_part = read_fuse(TEST_FLOW_CHECKS_SILICON_MAJOR_REVISION_BIT_OFFSET,
                                             TEST_FLOW_CHECKS_SILICON_MAJOR_REVISION_WIDTH) != 0;
        // Read override buffer from SPI Flash and apply to fuses if present
        status = read_override_from_spi();
        FUSE_ET_STATUS(FUSE_ET_TYPE_MAILBOX_REQUEST_OVERRIDES);
        const bool fuse_overrides_present = (status == SILIBS_SUCCESS);
        if (!is_fused_part && !fuse_overrides_present)
        {
            FUSE_ET_STATUS(FUSE_ET_TYPE_FUSED_NO_OVERRIDES);
            FPFW_RUNTIME_ASSERT(0);
        }
        else if (!is_fused_part && fuse_overrides_present)
        {
            printf(FUSE_NAME "Unfused part with fuse overrides in SPI. Applying overrides ignoring "
                             "per-fuse valids.\n");
            status = apply_fuse_override_ignoring_valids(DIE_0, Kingsgate_fuse_override_buffer_location);
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
            status = apply_fuse_override(DIE_0, Kingsgate_fuse_override_buffer_location);
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
int platform_fuse_distribution(void)
{
    int status = 0;
    const fuse_dist_exclude_range_t* fuse_dist_exclude_list = NULL;
    uint32_t exclude_list_count = 0;
    KNG_PLAT_ID plat_id = idsw_get_platform_sdv();
    switch (plat_id)
    {
    case PLATFORM_FPGA:
        printf("[KingGate_FUSE] Platform is FPGA\n");
        fuse_dist_get_exclusion_list(plat_id, &fuse_dist_exclude_list, &exclude_list_count);
        break;
    case PLATFORM_FPGA_LARGE:
        printf(FUSE_NAME "Platform is FPGA_Large\n");
        fuse_dist_get_exclusion_list(plat_id, &fuse_dist_exclude_list, &exclude_list_count);
        break;
    case PLATFORM_FPGA_LARGE_RVP:
        printf(FUSE_NAME "Platform is FPGA_Large_RVP\n");
        fuse_dist_get_exclusion_list(plat_id, &fuse_dist_exclude_list, &exclude_list_count);
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
        status = distribute_fuses(DIE_0, POST_HSP_DIST_MAJOR, POST_HSP_DIST_MINOR, fuse_dist_exclude_list, exclude_list_count);
        FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR3_MINOR0);
        if (status != SILIBS_SUCCESS)
        {
            return status;
        }
        printf(FUSE_NAME "Phase 0 fuse distribution complete\n");
        status = distribute_fuses(DIE_0, POST_HSP_DIST_MAJOR, MESH_INIT_MINOR, fuse_dist_exclude_list, exclude_list_count);
        FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR3_MINOR1);
        if (status != SILIBS_SUCCESS)
        {
            return status;
        }
        printf(FUSE_NAME "Phase 1 fuse distribution complete\n");

        status = distribute_fuses(DIE_0, POST_MESH_INIT_MAJOR, POST_MESH_INIT_MINOR, fuse_dist_exclude_list, exclude_list_count);
        FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR4_MINOR0);
        if (status != SILIBS_SUCCESS)
        {
            return status;
        }
        printf(FUSE_NAME "Phase 2 fuse distribution complete\n");

        status = distribute_fuses(DIE_0, POST_MESH_INIT_MAJOR, POST_BRIDGE_INIT_MINOR, fuse_dist_exclude_list, exclude_list_count);
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

    return status;
}
// This placeholder here is to verify the Fuse event trace log
void fuse_init()
{
    FUSE_ET_STATUS(FUSE_ET_TYPE_SVC_START);
}
