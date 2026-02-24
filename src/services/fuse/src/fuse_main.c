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
#include <fuse_main_i.h>
#include <fuse_struct.h>
#include <fuses_top_regs.h>
#include <icc_mhu.h> // for icc_mhu_packet_t
#include <icc_platform_defines.h>
#include <idhw.h>
#include <idsw.h> // SW platform id
#include <idsw_kng.h>
#include <kingsgate_fuse_defines.h> // Test revision get
#include <kng_icc_shared.h>
#include <kng_soc_constants.h> // for DIE_INSTANCE
#include <memory_map/mscp_exp_rmss_memory_map.h>
#include <mesh.h> // for mesh_get_hns_sds_vector_from_hns_sparring
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
#define FUSE_PAYLOAD_SIZE 7

/*------------- Typedefs -----------------*/
#define NUM_CSR_BACKED_CORE_FUSE_DESCRIPTORS (4)
#define MAX_BYTES_PER_FUSE                   8
#define MAX_BITS_PER_FUSE                    (MAX_BYTES_PER_FUSE * 8)
#define BITS_PER_BYTE                        (8)
#define PRIORITY_COUNT                       (13)

/*-------- Function Prototypes -----------*/
static bool platform_requires_fuse_distribution();
static int platform_fuse_copy_to_ram();

/*-- Declarations (Statics and globals) --*/

static fpfw_icc_base_ctx_t* icc_base_ctx_fuse;
static fpfw_icc_base_ctx_t* icc_base_ctx_die2die;
kng_fuse_disable_core_t DIE0_fuse_disable, DIE1_fuse_disable;
static kng_fuse_disable_core_t remote_die_core_disable = {0};
// Flag to track if remote die config already received before AP handover registration
static bool remote_die_cfg_received = false;

static fpfw_icc_base_send_req_t scp_send_params;
static fpfw_icc_base_recv_req_t scp_recv_params;

static uint32_t die0_core_disable_config_knob_31_0 = 0;
static uint32_t die0_core_disable_config_knob_63_32 = 0;
static uint32_t die0_core_disable_config_knob_95_64 = 0;

static uint32_t die1_core_disable_config_knob_31_0 = 0;
static uint32_t die1_core_disable_config_knob_63_32 = 0;
static uint32_t die1_core_disable_config_knob_95_64 = 0;
// Enable spare cores
static uint32_t die0_config_spare_core_en_31_0 = 0;
static uint32_t die0_config_spare_core_en_63_32 = 0;
static uint32_t die0_config_spare_core_en_95_64 = 0;

static uint32_t die1_config_spare_core_en_31_0 = 0;
static uint32_t die1_config_spare_core_en_63_32 = 0;
static uint32_t die1_config_spare_core_en_95_64 = 0;

struct ap_core_die_cfg_cb_t
{
    ap_core_die_cfg_cb cb;
    void* context;
};

typedef struct
{
    icc_mhu_header_t header;
    uint32_t payload[FUSE_PAYLOAD_SIZE];
} icc_mhu_d2d_fuse_packet_t;

static struct ap_core_die_cfg_cb_t ap_core_die_cfg_completion = {.cb = NULL, .context = NULL};

/*------------- Functions ----------------*/
void scp_remote_die_config_req_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_DBGPRINT_INFO(FUSE_NAME "Remote send completed 0x%" PRIx32 "\n", status);

    if (status == FPFW_ICC_TRANSPORT_STATUS_BUSY)
    {
        // Retry sending fuse info to remote die
        write_fuse_info_to_ap();
    }
    else if (FPFW_STATUS_SUCCEEDED(status))
    {
        //! Die 1 can complete the ap core handover
        BUG_ASSERT(ap_core_die_cfg_completion.cb != NULL && ap_core_die_cfg_completion.context != NULL);
        ap_core_die_cfg_completion.cb(ap_core_die_cfg_completion.context);
    }
    else
    {
        BUG_ASSERT_PARAM(false, status, "Failed to send remote die config");
    }
}

inline bool is_core_disabled(const kng_fuse_disable_core_t* fuse, uint32_t core)
{
    bool result = false;
    if (core < 32)
    {
        result = (fuse->fuse_dis_core_31_0 >> core) & 1U;
    }
    else if ((core >= 32) && (core < 64))
    {
        result = (fuse->fuse_dis_core_63_32 >> (core - 32)) & 1U;
    }
    else if ((core >= 64) && (core < 96))
    {
        result = ((fuse->fuse_dis_core_95_64 & 0x0f) >> (core - 64)) & 1U;
    }
    return result;
}

inline void disable_core(kng_fuse_disable_core_t* fuse, uint32_t core)
{
    if (core < 32)
    {
        fuse->fuse_dis_core_31_0 |= (1U << core);
    }
    else if ((core >= 31) && (core < 64))
    {
        fuse->fuse_dis_core_63_32 |= (1U << (core - 32));
    }
    else
    {
        fuse->fuse_dis_core_95_64 |= (1U << (core - 64));
    }
}

void fuse_disable_pick_algorithm(kng_fuse_disable_core_t* f)
{
    static const uint8_t column_map[5][12] = {{8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19},
                                              {20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
                                              {32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43},
                                              {44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55},
                                              {56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67}};
    static const uint8_t row_map[8][5] = {{10, 22, 34, 46, 58},
                                          {11, 23, 35, 47, 59},
                                          {12, 24, 36, 48, 60},
                                          {13, 25, 37, 49, 61},
                                          {14, 26, 38, 50, 62},
                                          {15, 27, 39, 51, 63},
                                          {16, 28, 40, 52, 64},
                                          {17, 29, 41, 53, 65}};
    static const uint8_t priority[PRIORITY_COUNT][3] = {{37, 2, 3},
                                                        {38, 2, 4},
                                                        {49, 3, 3},
                                                        {25, 1, 3},
                                                        {26, 1, 4},
                                                        {50, 3, 4},
                                                        {35, 2, 1},
                                                        {40, 2, 6},
                                                        {47, 3, 1},
                                                        {23, 1, 1},
                                                        {28, 1, 6},
                                                        {52, 3, 6},
                                                        {61, 4, 3}};
    for (int i = 0; i < 13; ++i)
    {
        uint32_t core = priority[i][0];
        uint32_t col_idx = priority[i][1];
        uint32_t row_idx = priority[i][2];

        if (is_core_disabled(f, core))
        {
            continue;
        }

        bool col_ok = true;
        for (int j = 0; j < 12; ++j)
        {
            if (is_core_disabled(f, column_map[col_idx][j]))
            {
                col_ok = false;
                break;
            }
        }
        if (!col_ok)
        {
            continue;
        }

        bool row_ok = true;
        for (int j = 0; j < 5; ++j)
        {
            if (is_core_disabled(f, row_map[row_idx][j]))
            {
                row_ok = false;
                break;
            }
        }
        if (!row_ok)
        {
            continue;
        }

        disable_core(f, core);
        break;
    }
}

// Adjust disabled cores to make total disabled cores to 2
// From ADO: 2772469 : SoC FW to enable 132 cores only in KNG SoC based on core selection algorithm
void fuse_disable_cores_to_66(kng_fuse_disable_core_t* p_fuse_disable)
{
    int total_disable_cores = __builtin_popcount(p_fuse_disable->fuse_dis_core_31_0) +
                              __builtin_popcount(p_fuse_disable->fuse_dis_core_63_32) +
                              __builtin_popcount(p_fuse_disable->fuse_dis_core_95_64 & 0x0f);
    if (total_disable_cores == 0)
    {
        FPFW_DBGPRINT_INFO(FUSE_NAME "total disable cores are 0 so we turn off Core26 and Core37\n");
        p_fuse_disable->fuse_dis_core_31_0 = p_fuse_disable->fuse_dis_core_31_0 | (1 << 26);
        p_fuse_disable->fuse_dis_core_63_32 = p_fuse_disable->fuse_dis_core_63_32 | (1 << 5);
        FPFW_DBGPRINT_INFO(FUSE_NAME "DIE [%d] : core31-0=0x%" PRIx32 " core63-32=0x%" PRIx32
                                     " core95-64=0x%" PRIx32 " \n",
                           idsw_get_die_id(),
                           p_fuse_disable->fuse_dis_core_31_0,
                           p_fuse_disable->fuse_dis_core_63_32,
                           p_fuse_disable->fuse_dis_core_95_64);
    }
    else if (total_disable_cores == 1)
    {
        FPFW_DBGPRINT_INFO(FUSE_NAME "DIE [%d] :  core31-0=0x%" PRIx32 " core63-32=0x%" PRIx32
                                     " core95-64=0x%" PRIx32 " \n",
                           idsw_get_die_id(),
                           p_fuse_disable->fuse_dis_core_31_0,
                           p_fuse_disable->fuse_dis_core_63_32,
                           p_fuse_disable->fuse_dis_core_95_64);
        FPFW_DBGPRINT_INFO(FUSE_NAME "total disable cores are 1 so enter pickup algorithm\n");
        fuse_disable_pick_algorithm(p_fuse_disable);
        FPFW_DBGPRINT_INFO(FUSE_NAME "After : core31-0=0x%" PRIx32 " core63-32=0x%" PRIx32
                                     " core95-64=0x%" PRIx32 " \n",
                           p_fuse_disable->fuse_dis_core_31_0,
                           p_fuse_disable->fuse_dis_core_63_32,
                           p_fuse_disable->fuse_dis_core_95_64);
    }
    else
    {
        FPFW_DBGPRINT_INFO(FUSE_NAME "equal or above 2 cores selected : core31-0=0x%" PRIx32
                                     " core63-32=0x%" PRIx32 " core95-64=0x%" PRIx32 " \n",
                           p_fuse_disable->fuse_dis_core_31_0,
                           p_fuse_disable->fuse_dis_core_63_32,
                           p_fuse_disable->fuse_dis_core_95_64);
    }
}

// write received information via sds
void save_remote_die_config_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    FPFW_DBGPRINT_INFO(FUSE_NAME "Remote die config received callback invoked with status 0x%" PRIx32 "\n", status);

    icc_mhu_d2d_fuse_packet_t* req_params = (icc_mhu_d2d_fuse_packet_t*)context;
    if (status != FPFW_STATUS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR(FUSE_NAME "Failed to receive remote die config request\n");
        FUSE_ET_STATUS_PARAM(FUSE_ET_TYPE_REMOTE_DIE_CONFIG_FAIL, status);
        return;
    }

    // Mark remote die config as received so AP handover can detect prior completion
    remote_die_cfg_received = true;

    remote_die_core_disable.fuse_dis_core_31_0 = req_params->payload[0];
    remote_die_core_disable.fuse_dis_core_63_32 = req_params->payload[1];
    remote_die_core_disable.fuse_dis_core_95_64 = req_params->payload[2];
    remote_die_core_disable.fuse_dis_core_127_96 = req_params->payload[3];

    uint32_t result =
        sds_block_creation(FUSE_DISABLE_CORE_DIE1_STRUCT_ID, FUSE_DISABLE_CORE_DIE1_SIZE, PLATFORM_SDS_REGION_ARSM_DIE0);
    BUG_ASSERT(result == KNG_SUCCESS);
    fuse_disable_cores_to_66(&remote_die_core_disable);
    result = sds_block_write(FUSE_DISABLE_CORE_DIE1_STRUCT_ID, &remote_die_core_disable, FUSE_DISABLE_CORE_DIE1_SIZE);
    BUG_ASSERT(result == KNG_SUCCESS);
    kng_hns_fuses_t remote_die_hns_fuses = {0};

    remote_die_hns_fuses.hns_fuses_31_0 = req_params->payload[4];
    remote_die_hns_fuses.hns_fuses_63_32 = req_params->payload[5];
    remote_die_hns_fuses.hns_fuses_95_64 = req_params->payload[6];

    result = sds_block_creation(HNS_FUSES_DIE1_STRUCT_ID, HNS_FUSES_SIZE, PLATFORM_SDS_REGION_ARSM_DIE0);
    BUG_ASSERT(result == KNG_SUCCESS);

    result = sds_block_write(HNS_FUSES_DIE1_STRUCT_ID, &remote_die_hns_fuses, HNS_FUSES_SIZE);
    BUG_ASSERT(result == KNG_SUCCESS);

    // Call the registered callback to notify the completion of remote die config
    if ((NULL != ap_core_die_cfg_completion.cb) && (NULL != ap_core_die_cfg_completion.context))
    {
        ap_core_die_cfg_completion.cb(ap_core_die_cfg_completion.context);
    }
}

bool fuse_has_remote_die_config(void)
{
    return remote_die_cfg_received;
}

int prepare_remote_die_config_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    // Static buffer to hold the received message
    static uint8_t scp_recv_payload[512] = {0};

    scp_recv_params.recv_cmd_code = ICC_SSI_STAGE_SDS_DIE_CFG; // Command code to listen for
    scp_recv_params.payload_buffer = scp_recv_payload;         // Buffer to receive the message
    scp_recv_params.buffer_size = sizeof(scp_recv_payload);    // Size of the buffer
    scp_recv_params.cb = save_remote_die_config_cb; // Callback function for handling the received message
    scp_recv_params.cb_ctx = scp_recv_payload;      // Context for the callback

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &scp_recv_params);

    BUG_ASSERT(status == KNG_SUCCESS);
    return status;
}

static bool platform_requires_fuse_distribution()
{
    bool status = false;
    KNG_PLAT_ID plat_id = idsw_get_platform_sdv();
    bool is_secure_state = (system_info_get_security_state() == HSP_SECURITY_STATE_SECURE);

    switch (plat_id)
    {
    case PLATFORM_RVP_EVT_SILICON:
    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        status = true; // Always distribute fuses regardless of security state
        break;
    case PLATFORM_RTL_SIM:
    case PLATFORM_EMU:
    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_2D:
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D_8C:
    case PLATFORM_SVP_SIM:
        if (is_secure_state)
        {
            status = false;
            FPFW_DBGPRINT_INFO(FUSE_NAME "Fuse distribution is not supported in secure state\n");
            FUSE_ET_STATUS_PARAM(FUSE_ET_TYPE_NO_FUSE_DISTRIBUTION, status);
        }
        else
        {
            status = true;
        }
        break;
    default:
        FPFW_DBGPRINT_INFO(FUSE_NAME "Fuse distribution not supported in FW for platform\n");
        FUSE_ET_STATUS_PARAM(FUSE_ET_TYPE_NO_FUSE_DISTRIBUTION, status);
        break;
    }
    return status;
}

static int platform_fuse_copy_to_ram()
{
    return fuse_dma_copy_to_ram_blocking();
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
        status = read_core_defect_fuses(&(p_fuse_disable->fuse_dis_core_95_64),
                                        &(p_fuse_disable->fuse_dis_core_63_32),
                                        &(p_fuse_disable->fuse_dis_core_31_0));
        if (status != SILIBS_SUCCESS)
        {
            FPFW_DBGPRINT_ERROR(FUSE_NAME "Unable to read cores fuses for DIE%d\n", p_die_num);
            FUSE_ET_STATUS_PARAM(FUSE_ET_TYPE_CORE_FUSES_READ_FAIL, status);
            return status;
        }

        // Apply Core disable knobs
        p_fuse_disable->fuse_dis_core_31_0 |= die0_core_disable_config_knob_31_0;
        p_fuse_disable->fuse_dis_core_63_32 |= die0_core_disable_config_knob_63_32;
        p_fuse_disable->fuse_dis_core_95_64 |= (die0_core_disable_config_knob_95_64 | 0xFFFFFFF0);
        p_fuse_disable->fuse_dis_core_127_96 = 0xFFFFFFFF;

        // Apply Spare Enable knobs
        p_fuse_disable->fuse_dis_core_31_0 &= ~die0_config_spare_core_en_31_0;
        p_fuse_disable->fuse_dis_core_63_32 &= ~die0_config_spare_core_en_63_32;
        p_fuse_disable->fuse_dis_core_95_64 &=
            ((p_fuse_disable->fuse_dis_core_95_64 & ~die0_config_spare_core_en_95_64) | 0xFFFFFFF0);
        p_fuse_disable->fuse_dis_core_127_96 = 0xFFFFFFFF;

        FPFW_DBGPRINT_INFO(FUSE_NAME "Save Core Disable fuse and knob info for DIE%d done\n", p_die_num);
    }
    else
    {
        p_fuse_disable = &DIE1_fuse_disable;
        p_die_num = 1;

        // Read Core fuses for die1
        status = read_core_defect_fuses(&(p_fuse_disable->fuse_dis_core_95_64),
                                        &(p_fuse_disable->fuse_dis_core_63_32),
                                        &(p_fuse_disable->fuse_dis_core_31_0));
        if (status != SILIBS_SUCCESS)
        {
            FPFW_DBGPRINT_ERROR(FUSE_NAME "Unable to read cores fuses for DIE%d\n", p_die_num);
            FUSE_ET_STATUS_PARAM(FUSE_ET_TYPE_CORE_FUSES_READ_FAIL, status);
            return status;
        }

        // Apply Core disable knobs
        p_fuse_disable->fuse_dis_core_31_0 |= die1_core_disable_config_knob_31_0;
        p_fuse_disable->fuse_dis_core_63_32 |= die1_core_disable_config_knob_63_32;
        p_fuse_disable->fuse_dis_core_95_64 |= (die1_core_disable_config_knob_95_64 | 0xFFFFFFF0);
        p_fuse_disable->fuse_dis_core_127_96 = 0xFFFFFFFF;

        // Apply Spare Enable knobs
        p_fuse_disable->fuse_dis_core_31_0 &= ~die1_config_spare_core_en_31_0;
        p_fuse_disable->fuse_dis_core_63_32 &= ~die1_config_spare_core_en_63_32;
        p_fuse_disable->fuse_dis_core_95_64 &=
            ((p_fuse_disable->fuse_dis_core_95_64 & ~die1_config_spare_core_en_95_64) | 0xFFFFFFF0);
        p_fuse_disable->fuse_dis_core_127_96 = 0xFFFFFFFF;

        FPFW_DBGPRINT_INFO(FUSE_NAME "Save Core Disable fuse and knob info for DIE%d done\n", p_die_num);
    }

    return SILIBS_SUCCESS;
}

void fuse_register_remote_die_cfg_completion_cb(ap_core_die_cfg_cb cb, void* ctx)
{
    if ((cb == NULL) || (ctx == NULL))
    {
        FPFW_DBGPRINT_ERROR(FUSE_NAME "AP core die cfg cb null\n");
        FUSE_ET_ERROR(FUSE_ET_TYPE_REMOTE_DIE_CONFIG_FAIL, 0);
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
        fuse_disable_cores_to_66(&DIE0_fuse_disable);
        result = sds_block_write(FUSE_DISABLE_CORE_DIE0_STRUCT_ID, &DIE0_fuse_disable, FUSE_DISABLE_CORE_DIE0_SIZE);
        BUG_ASSERT(result == KNG_SUCCESS);

        kng_hns_fuses_t DIE0_hns_fuses = {0x0};
        result = mesh_get_hns_sds_vector_from_hns_sparring(&DIE0_hns_fuses);
        if (result != SILIBS_SUCCESS)
        {
            FPFW_DBGPRINT_ERROR(FUSE_NAME "Unable to read HNS fuses for DIE%d\n", idsw_get_die_id());
            FUSE_ET_ERROR(FUSE_ET_TYPE_HNS_FUSES_READ_FAIL, idsw_get_die_id());
            return result;
        }
        result = sds_block_creation(HNS_FUSES_DIE0_STRUCT_ID, HNS_FUSES_SIZE, PLATFORM_SDS_REGION_ARSM_DIE0);
        BUG_ASSERT(result == KNG_SUCCESS);

        result = sds_block_write(HNS_FUSES_DIE0_STRUCT_ID, &DIE0_hns_fuses, HNS_FUSES_SIZE);
        BUG_ASSERT(result == KNG_SUCCESS);

        if (idhw_is_single_die_boot_en())
        {
            //! complete the ap core handover immediately
            ap_core_die_cfg_completion.cb(ap_core_die_cfg_completion.context);
        }
    }
    else
    {
        static uint8_t icc_mhu_send_die2die_payload[512] = {0};

        icc_mhu_d2d_fuse_packet_t* die2die_send_msg = (icc_mhu_d2d_fuse_packet_t*)icc_mhu_send_die2die_payload;

        // set up mhu header
        die2die_send_msg->header.msg_header.command = ICC_SSI_STAGE_SDS_DIE_CFG;
        die2die_send_msg->header.msg_header.payload_size = FUSE_PAYLOAD_SIZE * sizeof(uint32_t);

        // fill the payload with fuse disable info
        die2die_send_msg->payload[0] = DIE1_fuse_disable.fuse_dis_core_31_0;
        die2die_send_msg->payload[1] = DIE1_fuse_disable.fuse_dis_core_63_32;
        die2die_send_msg->payload[2] = DIE1_fuse_disable.fuse_dis_core_95_64;
        die2die_send_msg->payload[3] = DIE1_fuse_disable.fuse_dis_core_127_96;

        // This is the HNS fuses that will be sent to the Remote die
        // Send the HNS fuses to the Remote die
        kng_hns_fuses_t DIE1_hns_fuses = {0x0};
        result = mesh_get_hns_sds_vector_from_hns_sparring(&DIE1_hns_fuses);
        if (result != SILIBS_SUCCESS)
        {
            FPFW_DBGPRINT_ERROR(FUSE_NAME "Unable to read HNS fuses for DIE%d\n", idsw_get_die_id());
            FUSE_ET_ERROR(FUSE_ET_TYPE_HNS_FUSES_READ_FAIL, idsw_get_die_id());
            return result;
        }

        die2die_send_msg->payload[4] = DIE1_hns_fuses.hns_fuses_31_0;
        die2die_send_msg->payload[5] = DIE1_hns_fuses.hns_fuses_63_32;
        die2die_send_msg->payload[6] = DIE1_hns_fuses.hns_fuses_95_64;

        scp_send_params.payload_buffer = icc_mhu_send_die2die_payload;
        scp_send_params.buffer_size = sizeof(icc_mhu_send_die2die_payload);
        scp_send_params.cb = scp_remote_die_config_req_cb;
        scp_send_params.cb_ctx = icc_mhu_send_die2die_payload;

        fpfw_status_t icc_status = fpfw_icc_base_send(icc_base_ctx_die2die, &scp_send_params);
        if (icc_status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFW_DBGPRINT_ERROR(FUSE_NAME "send fuse info to primary die fail 0x%" PRIx32 "\n", icc_status);
            FUSE_ET_ERROR(FUSE_ET_TYPE_FUSE_ICC_BASE_SEND_FAIL, icc_status);
            BUG_ASSERT_PARAM((icc_status == FPFW_ICC_BASE_STATUS_SUCCESS), icc_status, icc_mhu_send_die2die_payload);
        }
    }
    FPFW_DBGPRINT_INFO(FUSE_NAME "DIE fuse info to ap successfully!\n");
    return result;
}

int platform_read_for_fuse(const uintptr_t fuse_store_addr, const uint64_t fuse_bit_offset, const uint32_t fuse_bit_size)
{
    uint64_t fuse_data = 0;
    if ((fuse_bit_size == 0) || (fuse_bit_size > MAX_BITS_PER_FUSE))
    {
        FPFW_DBGPRINT_ERROR(FUSE_NAME "Requested Fuse Size in bits not valid(Min:%d Max:%d bits) \n", 1, MAX_BITS_PER_FUSE);
        FUSE_ET_ERROR(FUSE_ET_TYPE_FUSE_SIZE_INVALID, fuse_bit_size);
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
    DIE_INSTANCE die_id = (DIE_INSTANCE)idsw_get_die_id();
    KNG_PLAT_ID plat_id = idsw_get_platform_sdv();

    // Skip for platforms that do not support fuses
    if (platform_requires_fuse_distribution())
    {
        // adjust fuse load address based on die_id
        uint32_t Kingsgate_fuse_override_buffer_location =
            (die_id == SOC_D0) ? (uint32_t)SCP_EXP_FUSE_DIE0_DATA_BASE : (uint32_t)SCP_EXP_FUSE_DIE1_DATA_BASE;

        // Cache base fuse data from efuse blocks into SoC RAM location
        status = platform_fuse_copy_to_ram();
        FUSE_ET_STATUS(FUSE_ET_TYPE_RAM_DMA_COPY);
        FPFW_DBGPRINT_INFO(FUSE_NAME "copy_to_ram status=%d\n", status);
        BUG_ASSERT_PARAM((status == SILIBS_SUCCESS), status, SILIBS_SUCCESS);

        /* Enable fuse feature once DMA fuse copy is successful */
        fuse_feature_enable(true);

        if (!config_get_fuse_knobs().fuse_ignore_ifwi_overrides)
        {
            const bool is_fused_part =
                (fuse_read(SILICON_ID_SILICON_MAJOR_REVISION_BIT_OFFSET, SILICON_ID_SILICON_MAJOR_REVISION_WIDTH) != 0);
            FPFW_DBGPRINT_INFO(FUSE_NAME "if fused part [%d] \n", is_fused_part);

            bool fuse_overrides_present = false;
            if (plat_id == PLATFORM_RVP_EVT_SILICON)
            {
                FUSE_ET_STATUS(FUSE_ET_TYPE_FUSED_WITH_OVERRIDES);
                FPFW_DBGPRINT_INFO(
                    FUSE_NAME
                    "Skipping fuse override read from HSP because it has already been loaded & will crash "
                    "HSP\n");
                fuse_overrides_present = true;
            }
            else
            {
                FUSE_ET_STATUS(FUSE_ET_TYPE_FUSED_NO_OVERRIDES);
                FPFW_DBGPRINT_INFO(FUSE_NAME "Non_support_machine! %d\n", plat_id);
                fuse_overrides_present = false; // For other platforms, we assume fuse overrides are not present
            }

            if (!is_fused_part && !fuse_overrides_present)
            {
                FUSE_ET_STATUS(FUSE_ET_TYPE_FUSED_NO_OVERRIDES);
                FPFW_DBGPRINT_INFO(FUSE_NAME "fuse no override\n");
                if (status != SILIBS_SUCCESS)
                {
                    FPFW_DBGPRINT_ERROR(FUSE_NAME "Fuse no override, copy_to_ram status=%d\n", status);
                    status = FUSE_NO_OVERRIDES;
                    FUSE_ET_STATUS_PARAM(FUSE_ET_TYPE_FUSED_NO_OVERRIDES, status);
                    return status;
                }
            }
            else if (!is_fused_part && fuse_overrides_present)
            {
                FPFW_DBGPRINT_INFO(FUSE_NAME
                                   "Unfused part with fuse overrides in SPI. Applying overrides ignoring "
                                   "per-fuse valids.\n");
                status = fuse_override_ignoring_valids(die_id, Kingsgate_fuse_override_buffer_location);
                FUSE_ET_STATUS(FUSE_ET_TYPE_FUSED_IGNORE_VALIDS);
                if (status != SILIBS_SUCCESS)
                {
                    FPFW_DBGPRINT_ERROR(FUSE_NAME "fuse_override_ignoring_valids fail!\n");
                    status = FUSE_ERROR_IGNORE_VALIDS;
                    FUSE_ET_STATUS_PARAM(FUSE_ET_TYPE_FUSED_IGNORE_VALIDS, status);
                    return status;
                }
            }
            else if (is_fused_part && !fuse_overrides_present)
            {
                FPFW_DBGPRINT_INFO(FUSE_NAME "Fused part with no fuse overrides in SPI.\n");
            }
            else
            {
                FPFW_DBGPRINT_INFO(FUSE_NAME
                                   "Fused part with fuse overrides in SPI. Applying all valid overrides.\n");
                status = fuse_override(die_id, Kingsgate_fuse_override_buffer_location);
                FUSE_ET_STATUS(FUSE_ET_TYPE_FUSED_WITH_OVERRIDES);

                if (status != SILIBS_SUCCESS)
                {
                    FPFW_DBGPRINT_ERROR(FUSE_NAME "fuse_override fail!\n");
                    status = FUSE_ERROR_WITH_OVERRIDES;
                    FUSE_ET_STATUS_PARAM(FUSE_ET_TYPE_FUSED_OVERRIDES_FAIL, status);
                    return status;
                }
            }
        }

        trigger_debugger_for_manual_overrides();

        if (!system_info_is_warm_start())
        {
            status = read_core_disables();
            if (status != SILIBS_SUCCESS)
            {
                FPFW_DBGPRINT_ERROR(FUSE_NAME "save disable knob failed\n");
                status = FUSE_ERROR_DISABLE_KNOB;
                FUSE_ET_STATUS_PARAM(FUSE_ET_TYPE_READ_CORE_DISABLE_FAIL, status);
                return status;
            }
        }
    }
    FUSE_ET_STATUS(FUSE_ET_TYPE_OVERRIDE_COMPLETE);
    FPFW_DBGPRINT_INFO(FUSE_NAME "Fuse Override complete\n");
    return status;
}

int platform_fuse_distribution(fuse_distribution_stage_t stage)
{
    int status = 0;
    const fuse_dist_exclude_range_t* fuse_dist_exclude_list = NULL;
    uint32_t exclude_list_count = 0;
    KNG_PLAT_ID plat_id = idsw_get_platform_sdv();
    DIE_INSTANCE die_id = (DIE_INSTANCE)idsw_get_die_id();
    bool is_secure_state = (system_info_get_security_state() == HSP_SECURITY_STATE_SECURE);
    switch (plat_id)
    {
    case PLATFORM_FPGA:
        FPFW_DBGPRINT_INFO(FUSE_NAME " Platform is FPGA\n");
        status = fuse_dist_get_exclusion_list_secure(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count, is_secure_state);
        break;
    case PLATFORM_FPGA_LARGE:
        FPFW_DBGPRINT_INFO(FUSE_NAME "Platform is FPGA_Large\n");
        status = fuse_dist_get_exclusion_list_secure(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count, is_secure_state);
        break;
    case PLATFORM_FPGA_LARGE_RVP:
        FPFW_DBGPRINT_INFO(FUSE_NAME "Platform is FPGA_Large_RVP\n");
        status = fuse_dist_get_exclusion_list_secure(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count, is_secure_state);
        break;
    case PLATFORM_RVP_EVT_SILICON:
        FPFW_DBGPRINT_INFO(FUSE_NAME "Platform is PLATFORM_RVP_EVT_SILICON\n");
        status = fuse_dist_get_exclusion_list_secure(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count, is_secure_state);
        break;
    case PLATFORM_EMU:
    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_2D:
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D_8C:
        FPFW_DBGPRINT_INFO(FUSE_NAME "Platform is EMU\n");
        status = fuse_dist_get_exclusion_list_secure(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count, is_secure_state);
        break;
    case PLATFORM_SVP_SIM:
        FPFW_DBGPRINT_INFO(FUSE_NAME "Platform is PLATFORM_SVP_SIM\n");
        status = fuse_dist_get_exclusion_list_secure(die_id, plat_id, &fuse_dist_exclude_list, &exclude_list_count, is_secure_state);
        break;
    default:
        status = SILIBS_E_SUPPORT;
        FUSE_ET_ERROR(FUSE_ET_TYPE_GET_EXCLUSION_LIST_FAIL, status);
        return status;
    }

    if (status != SILIBS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR(FUSE_NAME " GET_EXCLUSION_LIST Fail\n");
        status = FUSE_ERROR_GET_EXCLUSION_LIST;
        FUSE_ET_ERROR(FUSE_ET_TYPE_GET_EXCLUSION_LIST_FAIL, status);
        return status;
    }

    FPFW_DBGPRINT_INFO(FUSE_NAME "Fuse Distribution Start\n");
    FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_START);

    if (platform_requires_fuse_distribution())
    {
        if (stage == FUSE_DISTRIBUTION_STAGE_POST_HSP)
        {
            status = fuse_distribution(die_id, POST_HSP_DIST_MAJOR, POST_HSP_DIST_MINOR, fuse_dist_exclude_list, exclude_list_count);
            FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR3_MINOR0);
            if (status != SILIBS_SUCCESS)
            {
                FPFW_DBGPRINT_ERROR(FUSE_NAME "DISTRIBUTION_PHASE_MAJOR3_MINOR0 Fail\n");
                status = FUSE_ERROR_DISTRIBUTION_PHASE_MAJOR3_MINOR0;
                FUSE_ET_ERROR(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR3_MINOR0, status);
                return status;
            }
        }
        else if (stage == FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT)
        {
            status = fuse_distribution(die_id, POST_HSP_DIST_MAJOR, MESH_INIT_MINOR, fuse_dist_exclude_list, exclude_list_count);
            FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR3_MINOR1);
            if (status != SILIBS_SUCCESS)
            {
                FPFW_DBGPRINT_ERROR(FUSE_NAME "DISTRIBUTION_PHASE_MAJOR3_MINOR1 Fail\n");
                status = FUSE_ERROR_DISTRIBUTION_PHASE_MAJOR3_MINOR1;
                FUSE_ET_ERROR(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR3_MINOR1, status);
                return status;
            }
        }
        else if (stage == FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT)
        {
            status = fuse_distribution(die_id, POST_MESH_INIT_MAJOR, POST_MESH_INIT_MINOR, fuse_dist_exclude_list, exclude_list_count);
            FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR4_MINOR0);
            if (status != SILIBS_SUCCESS)
            {
                FPFW_DBGPRINT_ERROR(FUSE_NAME "DISTRIBUTION_PHASE_MAJOR4_MINOR0 Fail\n");
                status = FUSE_ERROR_DISTRIBUTION_PHASE_MAJOR4_MINOR0;
                FUSE_ET_ERROR(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR4_MINOR0, status);
                return status;
            }
        }
        else if (stage == FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT)
        {
            status = fuse_distribution(die_id, POST_MESH_INIT_MAJOR, POST_BRIDGE_INIT_MINOR, fuse_dist_exclude_list, exclude_list_count);
            FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR4_MINOR1);
            if (status != SILIBS_SUCCESS)
            {
                FPFW_DBGPRINT_ERROR(FUSE_NAME "DISTRIBUTION_PHASE_MAJOR4_MINOR1 Fail\n");
                status = FUSE_ERROR_DISTRIBUTION_PHASE_MAJOR4_MINOR1;
                FUSE_ET_ERROR(FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR4_MINOR1, status);
                return status;
            }
            FPFW_DBGPRINT_INFO(FUSE_NAME "Phase 3 fuse distribution complete\n");
        }

        FPFW_DBGPRINT_INFO(FUSE_NAME "Phase [%d] fuse distribution complete\n", stage);
        FUSE_ET_STATUS(FUSE_ET_TYPE_DISTRIBUTION_END);
    }
    else
    {
        FPFW_DBGPRINT_ERROR(FUSE_NAME "Platform does not support fuse distribution\n");
        FUSE_ET_ERROR(FUSE_ET_TYPE_NO_FUSE_DISTRIBUTION, status);
        return FUSE_ERROR_NO_DISTRIBUTION;
    }

    return status;
}

void fuse_init(fpfw_icc_base_ctx_t* icc_hspmbx_ctx)
{
    icc_base_ctx_fuse = icc_hspmbx_ctx;

    if (idsw_get_die_id() == DIE_0)
    {
        die0_core_disable_config_knob_31_0 = config_get_die0_core_disable_value_31_0();
        die0_core_disable_config_knob_63_32 = config_get_die0_core_disable_value_63_32();
        die0_core_disable_config_knob_95_64 = config_get_die0_core_disable_value_95_64();

        die0_config_spare_core_en_31_0 = config_get_die0_core_spare_en_31_0();
        die0_config_spare_core_en_63_32 = config_get_die0_core_spare_en_63_32();
        die0_config_spare_core_en_95_64 = config_get_die0_core_spare_en_95_64();
    }
    else
    {
        die1_core_disable_config_knob_31_0 = config_get_die1_core_disable_value_31_0();
        die1_core_disable_config_knob_63_32 = config_get_die1_core_disable_value_63_32();
        die1_core_disable_config_knob_95_64 = config_get_die1_core_disable_value_95_64();
        // core‐spare‐enable masks
        die1_config_spare_core_en_31_0 = config_get_die1_core_spare_en_31_0();
        die1_config_spare_core_en_63_32 = config_get_die1_core_spare_en_63_32();
        die1_config_spare_core_en_95_64 = config_get_die1_core_spare_en_95_64();
    }
    FUSE_ET_STATUS(FUSE_ET_TYPE_SVC_START);
}

void fuse_post_mesh_init(fpfw_icc_base_ctx_t* icc_die2die_ctx)
{
    icc_base_ctx_die2die = icc_die2die_ctx;
    if (!idhw_is_single_die_boot_en() && (idsw_get_die_id() == DIE_0))
    {
        // prepare the listener for remote die config request
        prepare_remote_die_config_listener(icc_base_ctx_die2die);
    }
}
void fuse_print_version(void)
{
    FPFW_DBGPRINT_INFO(FUSE_NAME "Current fuse artifacts version %d.%d.%d \n",
                       ((FUSE_ARTIFACT_VERSION >> 24) & 0xff),
                       ((FUSE_ARTIFACT_VERSION >> 12) & 0xfff),
                       (FUSE_ARTIFACT_VERSION & 0xfff));

    uint32_t soc_id = idhw_get_soc_id();
    FPFW_DBGPRINT_INFO(FUSE_NAME "Soc ID: 0x%08X\n", soc_id);
    FUSE_ET_LOG_TRACE_PARAM(FUSE_ET_TYPE_GET_SOC_ID, soc_id);
}
