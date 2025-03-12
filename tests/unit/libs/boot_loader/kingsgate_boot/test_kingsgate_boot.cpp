//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_kingsgate_boot.cpp
 * Unit test code for unpack image in boot loader
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for CmockaWrapperTest, TEST_FUNCTION, asse...
#include <boot_status.h>
#include <cstdint> // for uint32_t, uint8_t
#include <mock_scp_mcp_top.h>
#include <stddef.h>           // for size_t, NULL
#include <string.h>           // for memset
#include <vcruntime_string.h> // for memcmp

extern "C" {
#include <MboxPrimitives.h>
#include <fpfw_icc_base.h>        // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <hsp_firmware_headers.h> // for HSP_MAILBOX_CMD_BOOT_STATUS_NOTIFY, HSP_FIRMWARE_ID_SCP...
#include <hsp_interaction_i.h>    // for send_post_code , hsp_mailbox_init
#include <kingsgate_boot.h>       // for load_image, MSCP_CPU_SCP, MSCP_CPU_MCP
#include <unpack_image.h>         // for embed_image_header_t, EMBED_HEADER_TAG

/*-- Symbolic Constant Macros (defines) --*/

#define MEM_CMP_SUCCESS (0)
#define MAX_WBITS       (15)

#define ITCM_COMPRESSED_STRING_SIZE   (40)
#define ITCM_UNCOMPRESSED_STRING_SIZE (10)

#define DTCM_COMPRESSED_STRING_SIZE   (40)
#define DTCM_UNCOMPRESSED_STRING_SIZE (10)

#define RMSS_DATA_COMPRESSED_STRING_SIZE   (40)
#define RMSS_DATA_UNCOMPRESSED_STRING_SIZE (10)

static const char* ITCM_UNCOMPRESSED_STRING = "ITCM TEST\n";
static const char* DTCM_UNCOMPRESSED_STRING = "DTCM TEST\n";
static const char* RMSS_DATA_UNCOMPRESSED_STRING = "DTCM TEST\n";

/*------------- Typedefs -----------------*/
typedef struct main_image_test_s
{
    embed_image_header_t image_header;
    // uint8_t compressed_source[ITCM_COMPRESSED_STRING_SIZE + DTCM_COMPRESSED_STRING_SIZE];
    uint8_t compressed_itcm[ITCM_COMPRESSED_STRING_SIZE];
    uint8_t compressed_dtcm[DTCM_COMPRESSED_STRING_SIZE];
    uint8_t compressed_rmss[RMSS_DATA_COMPRESSED_STRING_SIZE];
} main_image_test_t;

typedef enum _HSP_MBOX_STATUS_EX
{
    HSP_MBOX_STATUS_NOT_FATAL = 0,
    HSP_MBOX_STATUS_FATAL,
    HSP_MBOX_STATUS_COMPLETE,
} HSP_MBOX_STATUS_EX;

/*------------------- Declarations (Statics and globals) --------------------*/
const uint32_t SCP_ITCM_RAM_SIZE = (512 * 1024);
const uint32_t SCP_DTCM_RAM_SIZE = (512 * 1024);
const uint32_t SCP_RMSS_DATA_SIZE = (192 * 1024);

const uint32_t MCP_ITCM_RAM_SIZE = (512 * 1024);
const uint32_t MCP_DTCM_RAM_SIZE = (512 * 1024);
const uint32_t MCP_RMSS_DATA_SIZE = (64 * 1024);

static uint8_t test_itc_ram[SCP_ITCM_RAM_SIZE];
static uint8_t test_dtc_ram[SCP_DTCM_RAM_SIZE];
static uint8_t test_rmss_data[SCP_RMSS_DATA_SIZE];

static HSP_BOOT_METADATA test_metadata;

static main_image_test_t test_main_image_data = {
    .image_header = {.embed_image_header_tag = EMBED_HEADER_TAG,
                     .itc_ram = {.compressed_offset = sizeof(embed_image_header_t),
                                 .compressed_size = ITCM_COMPRESSED_STRING_SIZE,
                                 .uncompressed_size = ITCM_UNCOMPRESSED_STRING_SIZE,
                                 .uncompressed_crc32 = 0x1A58A044},
                     .dtc_ram = {.compressed_offset = sizeof(embed_image_header_t) + ITCM_COMPRESSED_STRING_SIZE,
                                 .compressed_size = DTCM_COMPRESSED_STRING_SIZE,
                                 .uncompressed_size = DTCM_UNCOMPRESSED_STRING_SIZE,
                                 .uncompressed_crc32 = 0xEFA62BF4},
                     .rmss_data_ram = {.compressed_offset = sizeof(embed_image_header_t) + ITCM_COMPRESSED_STRING_SIZE + DTCM_COMPRESSED_STRING_SIZE,
                                       .compressed_size = RMSS_DATA_COMPRESSED_STRING_SIZE,
                                       .uncompressed_size = RMSS_DATA_UNCOMPRESSED_STRING_SIZE,
                                       .uncompressed_crc32 = 0xEFA62BF4}},
    .compressed_itcm = {0x1f, 0x8b, 0x08, 0x08, 0xea, 0x6a, 0x38, 0x66, 0x00, 0x03, 0x68, 0x6f, 0x70, 0x65,
                        0x2e, 0x74, 0x78, 0x74, 0x00, 0xf3, 0x0c, 0x71, 0xf6, 0x55, 0x08, 0x71, 0x0d, 0x0e,
                        0xe1, 0x02, 0x00, 0x44, 0xa0, 0x58, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x00},
    .compressed_dtcm = {0x1f, 0x8b, 0x08, 0x08, 0x0d, 0x6d, 0x38, 0x66, 0x00, 0x03, 0x68, 0x6f, 0x70, 0x65,
                        0x2e, 0x74, 0x78, 0x74, 0x00, 0x73, 0x09, 0x71, 0xf6, 0x55, 0x08, 0x71, 0x0d, 0x0e,
                        0xe1, 0x02, 0x00, 0xf4, 0x2b, 0xa6, 0xef, 0x0a, 0x00, 0x00, 0x00, 0x00},
    .compressed_rmss = {0x1f, 0x8b, 0x08, 0x08, 0x0d, 0x6d, 0x38, 0x66, 0x00, 0x03, 0x68, 0x6f, 0x70, 0x65,
                        0x2e, 0x74, 0x78, 0x74, 0x00, 0x73, 0x09, 0x71, 0xf6, 0x55, 0x08, 0x71, 0x0d, 0x0e,
                        0xe1, 0x02, 0x00, 0xf4, 0x2b, 0xa6, 0xef, 0x0a, 0x00, 0x00, 0x00, 0x00}};

static kingsgate_boot_config_t test_boot_config = {
    .data_src_base = (size_t)&test_main_image_data,
    .data_src_end = (size_t)&test_main_image_data.compressed_rmss[RMSS_DATA_COMPRESSED_STRING_SIZE - 1],
    .itc_ram_base = (size_t)&test_itc_ram[0],
    .itc_ram_size = SCP_ITCM_RAM_SIZE,
    .dtc_ram_base = (size_t)&test_dtc_ram[0],
    .dtc_ram_size = SCP_DTCM_RAM_SIZE,
    .rmss_data_base = (size_t)&test_rmss_data[0],
    .rmss_data_size = SCP_RMSS_DATA_SIZE,
    .boot_meta_base = (size_t)&test_metadata, // This boot config is expected to work correctly when passed as is to unpack_image
    .cpu_type = MSCP_CPU_SCP};

int ZLIB_WINDOW_SIZE = (16 + MAX_WBITS); // Window value passed to inflateInit2

struct kng_hsp_mailbox_boot_status_notify hsp_mbox_data = {
    .header = {.cmd = HSP_MAILBOX_CMD_BOOT_STATUS_NOTIFY, .seq = 0, .context = 0, .flags = 0},
    .id = HSP_FIRMWARE_ID_SCP,
    .boot_status = 0,
    .boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL};

FPFW_MBX_PAYLOAD mail_box_send_payload = {.payloadBuffer = &hsp_mbox_data,
                                          .payloadSize = (HSP_MBX_FIFO_DEPTH * sizeof(uint32_t))};

FPFW_MBX_REG_CONFIG mail_box_config = {.MbxFifoDepth = HSP_MBX_FIFO_DEPTH,
                                       .MbxMesgHandlingType = MBX_MESG_HANDLING_SINGLE_MESG_AT_A_TIME,
                                       .MbxImplementation = MBX_IMPL_POLLING,
                                       .MsgSizeBytes = (HSP_MBX_FIFO_DEPTH * sizeof(uint32_t)),
                                       .MbxBaseAddr = SCP_TOP_SCP2HSP_MAILBOX_ADDRESS};

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/
//
// Mocks
//

void __wrap___disable_irq()
{
}

void __wrap_system_info_init(fpfw_icc_base_ctx_t*)
{
}

int32_t __wrap_FpFwMailboxInit(PFPFW_MBX_REG_CONFIG pConfig, PFPFW_MBX_PRIMITIVE_CTX pMbxCtx)
{
    check_expected_ptr(pConfig);
    check_expected_ptr(pMbxCtx);

    check_expected(pConfig->MbxFifoDepth);
    check_expected(pConfig->MbxMesgHandlingType);
    check_expected(pConfig->MbxImplementation);
    check_expected(pConfig->MsgSizeBytes);
    check_expected(pConfig->MbxBaseAddr);

    return mock_type(int32_t);
}

int32_t __wrap_FpFwMailboxFlushFIFO(PFPFW_MBX_PRIMITIVE_CTX pMbxCtx)
{
    check_expected_ptr(pMbxCtx);

    return mock_type(int32_t);
}

int32_t __wrap_FpFwMailboxReceive(PFPFW_MBX_PRIMITIVE_CTX pMbxCtx, PFPFW_MBX_PAYLOAD pMessage)
{
    if (pMbxCtx == NULL || pMessage == NULL)
        return FPFW_MBX_E_INVALID_ARGS;

    return FPFW_MBX_SUCCESS;
}

int32_t __wrap_FpFwMailboxSend(PFPFW_MBX_PRIMITIVE_CTX pMbxCtx, PFPFW_MBX_PAYLOAD pMessage)
{
    uint32_t* post_code = NULL;
    uint32_t cmd = 0;
    uint32_t id = 0;
    uint32_t status = 0;
    uint32_t boot_code = 0;

    check_expected_ptr(pMbxCtx);
    check_expected_ptr(pMessage);

    check_expected_ptr(pMessage->payloadBuffer);
    check_expected(pMessage->payloadSize);

    post_code = (uint32_t*)(pMessage->payloadBuffer);
    cmd = post_code[0] & 0xFF;
    id = post_code[1];
    boot_code = post_code[2];
    status = post_code[3];

    check_expected(cmd);
    check_expected(id);
    check_expected(boot_code);
    check_expected(status);

    return mock_type(int32_t);
}

bool __wrap_system_info_is_hsp_present()
{
    return mock_type(bool);
}

//
// Tests
//

TEST_FUNCTION(test_hsp_mail_box_init_mbox_create_fail, nullptr, nullptr)
{
    // mailbox init fail
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);

    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_E_INVALID_ARGS);
    assert_false(hsp_mailbox_init(SCP_TOP_SCP2HSP_MAILBOX_ADDRESS));
}

TEST_FUNCTION(test_hsp_mail_box_init_mbox_flush_fail, nullptr, nullptr)
{
    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mail box flush fail
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_UNINITIALIZED);

    assert_false(hsp_mailbox_init(SCP_TOP_SCP2HSP_MAILBOX_ADDRESS));
}

TEST_FUNCTION(test_hsp_mail_box_init_api_fail_invalid_address, nullptr, nullptr)
{
    // mailbox invalid address
    uint32_t mail_box_address = 0;
    assert_false(hsp_mailbox_init(mail_box_address));
}

TEST_FUNCTION(test_hsp_mail_box_init_success, nullptr, nullptr)
{
    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_value(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth, mail_box_config.MbxFifoDepth);
    expect_value(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType, mail_box_config.MbxMesgHandlingType);
    expect_value(__wrap_FpFwMailboxInit, pConfig->MbxImplementation, mail_box_config.MbxImplementation);
    expect_value(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes, mail_box_config.MsgSizeBytes);
    expect_value(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr, mail_box_config.MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    assert_true(hsp_mailbox_init(SCP_TOP_SCP2HSP_MAILBOX_ADDRESS));
}

TEST_FUNCTION(test_send_post_code_scp_invalid_code, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    assert_false(send_post_code(BOOT_STATUS_CODE_SCP_MAX, true, false));
}

TEST_FUNCTION(test_send_post_code_mcp_invalid_min_code, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    assert_false(send_post_code(BOOT_STATUS_CODE_SCP_MAX, false, false));
}

TEST_FUNCTION(test_send_post_code_mcp_invalid_max_code, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    assert_false(send_post_code(BOOT_STATUS_CODE_MCP_MAX, false, false));
}

TEST_FUNCTION(test_send_post_code_scp_non_fatal_success, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    expect_any(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any(__wrap_FpFwMailboxSend, pMessage);
    expect_any(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_value(__wrap_FpFwMailboxSend, pMessage->payloadSize, mail_box_send_payload.payloadSize);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_OK;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);
    assert_true(send_post_code(BOOT_STATUS_CODE_SCP_OK, true, false));
}

TEST_FUNCTION(test_send_post_code_mcp_non_fatal_success, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    expect_any(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any(__wrap_FpFwMailboxSend, pMessage);
    expect_any(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_value(__wrap_FpFwMailboxSend, pMessage->payloadSize, mail_box_send_payload.payloadSize);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_OK;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);
    assert_true(send_post_code(BOOT_STATUS_CODE_MCP_OK, false, false));
}

TEST_FUNCTION(test_send_post_code_mcp_fatal_success, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    expect_any(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any(__wrap_FpFwMailboxSend, pMessage);
    expect_any(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_value(__wrap_FpFwMailboxSend, pMessage->payloadSize, mail_box_send_payload.payloadSize);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);
    assert_true(send_post_code(BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG, false, true));
}

TEST_FUNCTION(test_send_post_code_scp_fatal_success, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    expect_any(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any(__wrap_FpFwMailboxSend, pMessage);
    expect_any(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_value(__wrap_FpFwMailboxSend, pMessage->payloadSize, mail_box_send_payload.payloadSize);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);
    assert_true(send_post_code(BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG, true, true));
}

TEST_FUNCTION(test_send_post_code_scp_fail, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    expect_any_always(__wrap_FpFwMailboxSend, cmd);
    expect_any_always(__wrap_FpFwMailboxSend, id);
    expect_any_always(__wrap_FpFwMailboxSend, boot_code);
    expect_any_always(__wrap_FpFwMailboxSend, status);

    will_return_always(__wrap_FpFwMailboxSend, FPFW_MBX_FIFO_NOT_EMPTY);

    assert_false(send_post_code(BOOT_STATUS_CODE_SCP_OK, true, false));
}

TEST_FUNCTION(test_send_post_code_mcp_fail, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    expect_any_always(__wrap_FpFwMailboxSend, cmd);
    expect_any_always(__wrap_FpFwMailboxSend, id);
    expect_any_always(__wrap_FpFwMailboxSend, boot_code);
    expect_any_always(__wrap_FpFwMailboxSend, status);

    will_return_always(__wrap_FpFwMailboxSend, FPFW_MBX_FIFO_NOT_EMPTY);

    assert_false(send_post_code(BOOT_STATUS_CODE_MCP_OK, false, false));
}

TEST_FUNCTION(test_null_boot_config, nullptr, nullptr)
{
    kingsgate_boot_config_t* config = NULL;
    assert_null(load_image(config));
}

TEST_FUNCTION(test_boot_config_invalid_cpu, nullptr, nullptr)
{
    // Error injecting invalid cpu type for MSCP
    test_boot_config.cpu_type = MSCP_CPU_INVALID;
    assert_null(load_image(&test_boot_config));
}

TEST_FUNCTION(test_load_image_mailbox_init_fail, nullptr, nullptr)
{
    // Error injecting HSP mailbox init fail
    test_boot_config.cpu_type = MSCP_CPU_SCP;

    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);

    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);

    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_E_INVALID_ARGS);

    assert_null(load_image(&test_boot_config));
}

TEST_FUNCTION(test_scp_boot_config_src_base_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting invalid main image section start address
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.data_src_base = 0x0;

    // mail box create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mail box flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP start code send success
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP data src base error
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));

    // Reverting to correct value
    test_boot_config.data_src_base = (size_t)&test_main_image_data;
}

TEST_FUNCTION(test_scp_boot_config_src_end_size_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting invalid main image end section address
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.data_src_end = 0x0;

    // mail box create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mail box flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP send start code success
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP send image size error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.data_src_end = (size_t)&test_main_image_data.compressed_rmss[RMSS_DATA_COMPRESSED_STRING_SIZE - 1];
}

TEST_FUNCTION(test_scp_boot_config_image_size_zero_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect main image size
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.data_src_base = test_boot_config.data_src_end;

    // mailbox init success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP send image size error
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.data_src_base = (size_t)&test_main_image_data;
}

TEST_FUNCTION(test_scp_boot_config_itc_ram_base_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect ITCM ram base
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.itc_ram_base = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP send start code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP send ITCRAM base error
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.itc_ram_base = (size_t)&test_itc_ram[0];
}

TEST_FUNCTION(test_scp_boot_config_itc_ram_size_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect ITCM ram size
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.itc_ram_size = 0x0;

    // mailbox init success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP send ITC RAM size error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.itc_ram_size = SCP_ITCM_RAM_SIZE;
}

TEST_FUNCTION(test_scp_boot_config_dtc_ram_base_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect DTCM ram base
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.dtc_ram_base = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP send start code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP send DTC RAM base error
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.dtc_ram_base = (size_t)&test_dtc_ram[0];
}

TEST_FUNCTION(test_scp_boot_config_dtc_ram_size_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect DTCM ram size
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.dtc_ram_size = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP send DTC RAM size error
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.dtc_ram_size = SCP_DTCM_RAM_SIZE;
}

TEST_FUNCTION(test_scp_boot_config_rmss_data_ram_base_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect DTCM ram base
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.rmss_data_base = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP DTC RAM base error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.rmss_data_base = (size_t)&test_rmss_data[0];
}

TEST_FUNCTION(test_scp_boot_config_rmss_data_ram_size_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect ITCM ram size
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.rmss_data_size = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP ITC RAM size error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);
    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.rmss_data_size = SCP_RMSS_DATA_SIZE;
}

TEST_FUNCTION(test_scp_boot_config_meta_base_null_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect boot meta data base address
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.boot_meta_base = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP boot meta base error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.boot_meta_base = (size_t)&test_metadata;
}

TEST_FUNCTION(test_mcp_boot_config_src_base_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting invalid main image section start address
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.data_src_base = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush sucess
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP data src base error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.data_src_base = (size_t)&test_main_image_data;
}

TEST_FUNCTION(test_mcp_boot_config_src_end_size_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting invalid main image end section address
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.data_src_end = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP data src end error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.data_src_end = (size_t)&test_main_image_data.compressed_rmss[RMSS_DATA_COMPRESSED_STRING_SIZE - 1];
}

TEST_FUNCTION(test_mcp_boot_config_image_size_zero_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect main image size
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.data_src_base = test_boot_config.data_src_end;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush fifo success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP image size error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.data_src_base = (size_t)&test_main_image_data;
}

TEST_FUNCTION(test_mcp_boot_config_itc_ram_base_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect ITCM ram base
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.itc_ram_base = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP ITC RAM base error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.itc_ram_base = (size_t)&test_itc_ram[0];
}

TEST_FUNCTION(test_mcp_boot_config_itc_ram_size_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect ITCM ram size
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.itc_ram_size = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP ITC RAM size error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);
    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.itc_ram_size = MCP_ITCM_RAM_SIZE;
}

TEST_FUNCTION(test_mcp_boot_config_dtc_ram_base_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect DTCM ram base
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.dtc_ram_base = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP DTC RAM base error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.dtc_ram_base = (size_t)&test_dtc_ram[0];
}

TEST_FUNCTION(test_mcp_boot_config_dtc_ram_size_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect DTCM ram size
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.dtc_ram_size = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP DTC RAM size error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.dtc_ram_size = MCP_DTCM_RAM_SIZE;
}

TEST_FUNCTION(test_mcp_boot_config_rmss_data_ram_base_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect DTCM ram base
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.rmss_data_base = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP DTC RAM base error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.rmss_data_base = (size_t)&test_rmss_data[0];
}

TEST_FUNCTION(test_mcp_boot_config_rmss_data_ram_size_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect ITCM ram size
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.rmss_data_size = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP ITC RAM size error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);
    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.rmss_data_size = MCP_RMSS_DATA_SIZE;
}

TEST_FUNCTION(test_mcp_boot_config_meta_base_null_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect boot meta data base address
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.boot_meta_base = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP boot meta error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.boot_meta_base = (size_t)&test_metadata;
}

TEST_FUNCTION(test_scp_load_image_error_hsp_send_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect boot meta data base address
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.boot_meta_base = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP send start code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP boot meta error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_any_always(__wrap_FpFwMailboxSend, cmd);
    expect_any_always(__wrap_FpFwMailboxSend, id);
    expect_any_always(__wrap_FpFwMailboxSend, boot_code);
    expect_any_always(__wrap_FpFwMailboxSend, status);
    will_return_always(__wrap_FpFwMailboxSend, FPFW_MBX_FIFO_NOT_EMPTY);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value

    test_boot_config.boot_meta_base = (size_t)&test_metadata;
}

TEST_FUNCTION(test_mcp_load_image_error_hsp_send_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.boot_meta_base = 0x0;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP Boot meta error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_any_always(__wrap_FpFwMailboxSend, cmd);
    expect_any_always(__wrap_FpFwMailboxSend, id);
    expect_any_always(__wrap_FpFwMailboxSend, boot_code);
    expect_any_always(__wrap_FpFwMailboxSend, status);

    // Mailbox HSP send fails
    will_return_always(__wrap_FpFwMailboxSend, FPFW_MBX_FIFO_NOT_EMPTY);

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.boot_meta_base = (size_t)&test_metadata;
}

TEST_FUNCTION(test_scp_start_hsp_send_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect boot meta data base address
    test_boot_config.cpu_type = MSCP_CPU_SCP;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP send start code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_any_always(__wrap_FpFwMailboxSend, cmd);
    expect_any_always(__wrap_FpFwMailboxSend, id);
    expect_any_always(__wrap_FpFwMailboxSend, boot_code);
    expect_any_always(__wrap_FpFwMailboxSend, status);

    will_return_always(__wrap_FpFwMailboxSend, FPFW_MBX_FIFO_NOT_EMPTY);

    assert_null(load_image(&test_boot_config));
}

TEST_FUNCTION(test_mcp_start_hsp_send_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    test_boot_config.cpu_type = MSCP_CPU_MCP;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_any_always(__wrap_FpFwMailboxSend, cmd);
    expect_any_always(__wrap_FpFwMailboxSend, id);
    expect_any_always(__wrap_FpFwMailboxSend, boot_code);
    expect_any_always(__wrap_FpFwMailboxSend, status);
    will_return_always(__wrap_FpFwMailboxSend, FPFW_MBX_FIFO_NOT_EMPTY);
    assert_null(load_image(&test_boot_config));
}

TEST_FUNCTION(test_scp_irq_disable_hsp_send_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    test_boot_config.cpu_type = MSCP_CPU_SCP;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP IRQ disabled code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_IRQ_DISABLED;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_any_always(__wrap_FpFwMailboxSend, cmd);
    expect_any_always(__wrap_FpFwMailboxSend, id);
    expect_any_always(__wrap_FpFwMailboxSend, boot_code);
    expect_any_always(__wrap_FpFwMailboxSend, status);

    will_return_always(__wrap_FpFwMailboxSend, FPFW_MBX_FIFO_NOT_EMPTY);

    assert_null(load_image(&test_boot_config));
}

TEST_FUNCTION(test_mcp_irq_disable_hsp_send_failure, nullptr, nullptr)
{
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // Error injecting incorrect boot meta data base address
    test_boot_config.cpu_type = MSCP_CPU_MCP;

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_IRQ_DISABLED;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_any_always(__wrap_FpFwMailboxSend, cmd);
    expect_any_always(__wrap_FpFwMailboxSend, id);
    expect_any_always(__wrap_FpFwMailboxSend, boot_code);
    expect_any_always(__wrap_FpFwMailboxSend, status);

    will_return_always(__wrap_FpFwMailboxSend, FPFW_MBX_FIFO_NOT_EMPTY);

    assert_null(load_image(&test_boot_config));
}

TEST_FUNCTION(test_scp_boot_reason_hsp_send_fail, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_IRQ_DISABLED;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_COLD_BOOT;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_any_always(__wrap_FpFwMailboxSend, cmd);
    expect_any_always(__wrap_FpFwMailboxSend, id);
    expect_any_always(__wrap_FpFwMailboxSend, boot_code);
    expect_any_always(__wrap_FpFwMailboxSend, status);

    will_return_always(__wrap_FpFwMailboxSend, FPFW_MBX_FIFO_NOT_EMPTY);

    assert_null(load_image(&test_boot_config));
}

TEST_FUNCTION(test_mcp_boot_reason_hsp_send_fail, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    will_return_always(__wrap_system_info_is_hsp_present, true);
    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_IRQ_DISABLED;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_COLD_BOOT;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_any_always(__wrap_FpFwMailboxSend, cmd);
    expect_any_always(__wrap_FpFwMailboxSend, id);
    expect_any_always(__wrap_FpFwMailboxSend, boot_code);
    expect_any_always(__wrap_FpFwMailboxSend, status);

    will_return_always(__wrap_FpFwMailboxSend, FPFW_MBX_FIFO_NOT_EMPTY);

    assert_null(load_image(&test_boot_config));
}

TEST_FUNCTION(test_scp_cold_boot_unpack_image_fail, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_metadata.ResetReason &= ~(BITMASK_WARM_BOOT);
    test_main_image_data.compressed_itcm[0] = 0x0; // Corrupt compressed ITCM header to cause decompress() API to fail

    will_return_always(__wrap_system_info_is_hsp_present, true);

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_IRQ_DISABLED;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_COLD_BOOT;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));

    test_main_image_data.compressed_itcm[0] = 0x1f; // Restore compressed data header
}

TEST_FUNCTION(test_scp_warm_boot_unpack_image_fail, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_main_image_data.compressed_itcm[0] = 0x0; // Corrupt compressed ITCM header to cause decompress() API to fail
    test_metadata.ResetReason |= BITMASK_WARM_BOOT;

    will_return_always(__wrap_system_info_is_hsp_present, true);

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_IRQ_DISABLED;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_WARM_BOOT;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP unpack fail error code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));

    test_main_image_data.compressed_itcm[0] = 0x1f; // Restore compressed data header
}

TEST_FUNCTION(test_mcp_cold_boot_unpack_image_fail, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_metadata.ResetReason &= ~(BITMASK_WARM_BOOT);
    test_main_image_data.compressed_itcm[0] = 0x0; // Corrupt compressed ITCM header to cause decompress() API to fail

    will_return_always(__wrap_system_info_is_hsp_present, true);

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP IRQ disable code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_IRQ_DISABLED;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP cold boot reason send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_COLD_BOOT;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP unpack image fail error code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));

    test_main_image_data.compressed_itcm[0] = 0x1f; // Restore compressed data header
}

TEST_FUNCTION(test_mcp_warm_boot_unpack_image_fail, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_main_image_data.compressed_itcm[0] = 0x0; // Corrupt compressed ITCM header to cause decompress() API to fail
    test_metadata.ResetReason |= BITMASK_WARM_BOOT;

    will_return_always(__wrap_system_info_is_hsp_present, true);

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP irq disable code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_IRQ_DISABLED;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP warm boot reason code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_WARM_BOOT;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP unpack image fail code
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_E_BOOT_CONFIG;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_null(load_image(&test_boot_config));

    test_main_image_data.compressed_itcm[0] = 0x1f; // Restore compressed data header
}

TEST_FUNCTION(test_scp_boot_success, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_metadata.ResetReason &= ~(BITMASK_WARM_BOOT);

    will_return_always(__wrap_system_info_is_hsp_present, true);

    memset((void*)test_boot_config.itc_ram_base, 0x0, ITCM_UNCOMPRESSED_STRING_SIZE);
    memset((void*)test_boot_config.dtc_ram_base, 0x0, DTCM_UNCOMPRESSED_STRING_SIZE);
    memset((void*)test_boot_config.rmss_data_base, 0x0, RMSS_DATA_UNCOMPRESSED_STRING_SIZE);

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // SCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP irq disable code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_IRQ_DISABLED;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // SCP cold boot reason send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_SCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_SCP_COLD_BOOT;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_non_null(load_image(&test_boot_config));

    assert_int_equal(memcmp((void*)test_boot_config.itc_ram_base, ITCM_UNCOMPRESSED_STRING, ITCM_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
    assert_int_equal(memcmp((void*)test_boot_config.dtc_ram_base, DTCM_UNCOMPRESSED_STRING, DTCM_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
    assert_int_equal(memcmp((void*)test_boot_config.rmss_data_base, RMSS_DATA_UNCOMPRESSED_STRING, RMSS_DATA_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
}

TEST_FUNCTION(test_mcp_boot_success, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_metadata.ResetReason &= ~(BITMASK_WARM_BOOT);

    will_return_always(__wrap_system_info_is_hsp_present, true);

    memset((void*)test_boot_config.itc_ram_base, 0x0, ITCM_UNCOMPRESSED_STRING_SIZE);
    memset((void*)test_boot_config.dtc_ram_base, 0x0, DTCM_UNCOMPRESSED_STRING_SIZE);
    memset((void*)test_boot_config.rmss_data_base, 0x0, RMSS_DATA_UNCOMPRESSED_STRING_SIZE);

    // mailbox create success
    expect_any(__wrap_FpFwMailboxInit, pConfig);
    expect_any(__wrap_FpFwMailboxInit, pMbxCtx);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxFifoDepth);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxMesgHandlingType);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxImplementation);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MsgSizeBytes);
    expect_any(__wrap_FpFwMailboxInit, pConfig->MbxBaseAddr);
    will_return(__wrap_FpFwMailboxInit, FPFW_MBX_SUCCESS);

    // mailbox flush success
    expect_any(__wrap_FpFwMailboxFlushFIFO, pMbxCtx);
    will_return(__wrap_FpFwMailboxFlushFIFO, FPFW_MBX_SUCCESS);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadSize);

    // MCP start code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_START;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP IRQ disable code send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_IRQ_DISABLED;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    // MCP cold boot reason send
    hsp_mbox_data.id = HSP_FIRMWARE_ID_MCP;
    hsp_mbox_data.boot_status = BOOT_STATUS_CODE_MCP_COLD_BOOT;
    hsp_mbox_data.boot_status_ex = HSP_MBOX_STATUS_NOT_FATAL;

    expect_value(__wrap_FpFwMailboxSend, cmd, (uint32_t)(hsp_mbox_data.header.cmd & 0xFFFF));
    expect_value(__wrap_FpFwMailboxSend, id, (uint32_t)hsp_mbox_data.id);
    expect_value(__wrap_FpFwMailboxSend, boot_code, (uint32_t)hsp_mbox_data.boot_status);
    expect_value(__wrap_FpFwMailboxSend, status, (uint32_t)hsp_mbox_data.boot_status_ex);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);

    assert_non_null(load_image(&test_boot_config));
    assert_int_equal(memcmp((void*)test_boot_config.itc_ram_base, ITCM_UNCOMPRESSED_STRING, ITCM_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
    assert_int_equal(memcmp((void*)test_boot_config.dtc_ram_base, DTCM_UNCOMPRESSED_STRING, DTCM_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
    assert_int_equal(memcmp((void*)test_boot_config.rmss_data_base, RMSS_DATA_UNCOMPRESSED_STRING, RMSS_DATA_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
}

} // extern c end
