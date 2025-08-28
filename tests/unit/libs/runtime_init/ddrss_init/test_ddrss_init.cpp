//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ddrss_init.cpp
 * DDRSS init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h>   // for FPFW_UNUSED
#include <ddr_manager.h> // for ddr_manager_init, ddr_service_context_t
#include <ddrss.h>
#include <ddrss_lib.h>
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <hsp_firmware_headers.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kingsgate_hsp_mailbox_commands.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_ddr;
extern fpfw_init_component_t _fpfw_component_ddr_pcr;

static uint32_t dummy_icc_ctx = 0;

/*------------- Functions ----------------*/

//
// Mocks
//

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

KNG_DIE_ID __wrap_idhw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_ddr_manager_init(ddr_service_context_t* pddr_service_ctx, ddr_service_config_t* pconfig, fpfw_icc_base_ctx_t* icc_ctx)
{
    FPFW_UNUSED(pddr_service_ctx);
    FPFW_UNUSED(pconfig);
    FPFW_UNUSED(icc_ctx);
}

uint32_t __wrap_FPFwCoreInterruptEnableVector(uint32_t interrupt_id)
{
    FPFW_UNUSED(interrupt_id);
    return 0;
}

uint32_t __wrap_FPFwCoreInterruptRegisterCallback(uint32_t interrupt_id)
{
    FPFW_UNUSED(interrupt_id);
    return 0;
}

bool __wrap_system_info_is_warm_start(void)
{
    return mock_type(bool);
}

uintptr_t __wrap_ddrss_atu_map_cfg_space(uint32_t die_num)
{
    FPFW_UNUSED(die_num);
    return mock_type(uintptr_t);
}

void __wrap_ddrss_atu_unmap_cfg_space(uint32_t die_num)
{
    FPFW_UNUSED(die_num);
    function_called();
}

int __wrap_ddrss_set_die_base(uint32_t die_id, uintptr_t base)
{
    FPFW_UNUSED(die_id);
    FPFW_UNUSED(base);
    return mock_type(int);
}

void __wrap_pcr_ddrss_configure_clock_and_pcr_reset(uint32_t ddrss_mask, uint8_t die_idx)
{
    FPFW_UNUSED(ddrss_mask);
    FPFW_UNUSED(die_idx);
    function_called();
}

bool __wrap_config_get_fips_kat_en(void)
{
    return mock_type(bool);
}

int __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(int);
}

void* __wrap_fpfw_init_get_handle(const char* id)
{
    FPFW_UNUSED(id);
    return &dummy_icc_ctx;
}

fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size, size_t* output_recv_bytes)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(payload_buffer);
    FPFW_UNUSED(output_recv_bytes);
    check_expected(buffer_size);

    kng_hsp_mailbox_msg* send_msg = (kng_hsp_mailbox_msg*)payload_buffer;
    kng_hsp_mailbox_msg* recv_msg = (kng_hsp_mailbox_msg*)payload_buffer;

    if (send_msg->header.cmd == HSP_MAILBOX_CMD_DDRSS_DEPLOY_FIPS_KEYS_REQ)
    {
        recv_msg->header.cmd = HSP_MAILBOX_CMD_DDRSS_DEPLOY_FIPS_KEYS_RSP;
        recv_msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
        *output_recv_bytes = sizeof(kng_hsp_mailbox_msg);
    }
    else if (send_msg->header.cmd == HSP_MAILBOX_CMD_DDRSS_FIPS_KEY_TEST_STATUS_NOTIFY)
    {
        recv_msg->header.cmd = HSP_MAILBOX_CMD_DDRSS_FIPS_KEY_TEST_STATUS_NOTIFY_RSP;
        recv_msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
        *output_recv_bytes = sizeof(kng_hsp_mailbox_msg);
    }
    else if (send_msg->header.cmd == HSP_MAILBOX_CMD_DDRSS_DEPLOY_PROD_KEYS_REQ)
    {
        recv_msg->header.cmd = HSP_MAILBOX_CMD_DDRSS_DEPLOY_PROD_KEYS_RSP;
        recv_msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
        *output_recv_bytes = sizeof(kng_hsp_mailbox_msg);
    }

    return mock_type(fpfw_status_t);
}

//
// Tests
//
TEST_FUNCTION(test_ddr_pcr_init_warm_start, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_system_info_is_warm_start, true);

    // Call API under test
    _fpfw_component_ddr_pcr.init_fn();
}

TEST_FUNCTION(test_ddr_pcr_init, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_system_info_is_warm_start, false);
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);

    // Inside prod_ddrss_pcr_init
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_ddrss_atu_map_cfg_space, 0x12345678);
    will_return(__wrap_ddrss_set_die_base, SILIBS_SUCCESS);
    expect_function_call(__wrap_pcr_ddrss_configure_clock_and_pcr_reset);
    expect_function_call(__wrap_ddrss_atu_unmap_cfg_space);

    // Call API under test
    _fpfw_component_ddr_pcr.init_fn();
}

TEST_FUNCTION(test_fips_kat_enable_ddrss_load_crypto_key, nullptr, nullptr)
{
    uint32_t mc = 0;
    uint32_t msg = 0;
    uint32_t timeout_us = 0;
    int sts = SILIBS_E_SUPPORT;
    KNG_PLAT_ID platform_id = PLATFORM_FPGA_LARGE;

    // Set up expectations : fips_kat_enable on fpga : DDRSS_HSP_MSG_FIPS_KEYS_LOADED
    msg = DDRSS_HSP_MSG_FIPS_KEYS_LOADED;
    will_return(__wrap_config_get_fips_kat_en, true);
    will_return(__wrap_idsw_get_platform_sdv, platform_id);

    expect_value(__wrap_fpfw_icc_base_send_recv_sync, buffer_size, (uint32_t)sizeof(kng_hsp_mailbox_msg));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    sts = ddrss_load_crypto_key(mc, msg, timeout_us);
    assert_int_equal(sts, SILIBS_SUCCESS);

    // Set up expectations : fips_kat_enable on fpga : DDRSS_HSP_MSG_FIPS_KEYS_TEST_COMPLETE
    msg = DDRSS_HSP_MSG_FIPS_KEYS_TEST_COMPLETE;
    will_return(__wrap_config_get_fips_kat_en, true);
    will_return(__wrap_idsw_get_platform_sdv, platform_id);

    expect_value(__wrap_fpfw_icc_base_send_recv_sync, buffer_size, (uint32_t)sizeof(kng_hsp_mailbox_msg));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    sts = ddrss_load_crypto_key(mc, msg, timeout_us);
    assert_int_equal(sts, SILIBS_SUCCESS);

    // Set up expectations : fips_kat_enable on fpga : DDRSS_HSP_MSG_PROD_KEYS_LOADED
    msg = DDRSS_HSP_MSG_PROD_KEYS_LOADED;
    will_return(__wrap_config_get_fips_kat_en, true);
    will_return(__wrap_idsw_get_platform_sdv, platform_id);

    expect_value(__wrap_fpfw_icc_base_send_recv_sync, buffer_size, (uint32_t)sizeof(kng_hsp_mailbox_msg));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    sts = ddrss_load_crypto_key(mc, msg, timeout_us);
    assert_int_equal(sts, SILIBS_SUCCESS);

    // Set up expectations : fips_kat_enable on silicon : DDRSS_HSP_MSG_FIPS_KEYS_LOADED
    platform_id = PLATFORM_RVP_EVT_SILICON;
    msg = DDRSS_HSP_MSG_FIPS_KEYS_LOADED;
    will_return(__wrap_config_get_fips_kat_en, true);
    will_return(__wrap_idsw_get_platform_sdv, platform_id);

    expect_value(__wrap_fpfw_icc_base_send_recv_sync, buffer_size, (uint32_t)sizeof(kng_hsp_mailbox_msg));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    sts = ddrss_load_crypto_key(mc, msg, timeout_us);
    assert_int_equal(sts, SILIBS_SUCCESS);

    // Set up expectations : fips_kat_enable on silicon : DDRSS_HSP_MSG_FIPS_KEYS_TEST_COMPLETE
    msg = DDRSS_HSP_MSG_FIPS_KEYS_TEST_COMPLETE;
    will_return(__wrap_config_get_fips_kat_en, true);
    will_return(__wrap_idsw_get_platform_sdv, platform_id);

    expect_value(__wrap_fpfw_icc_base_send_recv_sync, buffer_size, (uint32_t)sizeof(kng_hsp_mailbox_msg));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    sts = ddrss_load_crypto_key(mc, msg, timeout_us);
    assert_int_equal(sts, SILIBS_SUCCESS);

    // Set up expectations : fips_kat_enable on silicon : DDRSS_HSP_MSG_PROD_KEYS_LOADED
    msg = DDRSS_HSP_MSG_PROD_KEYS_LOADED;
    will_return(__wrap_config_get_fips_kat_en, true);
    will_return(__wrap_idsw_get_platform_sdv, platform_id);

    expect_value(__wrap_fpfw_icc_base_send_recv_sync, buffer_size, (uint32_t)sizeof(kng_hsp_mailbox_msg));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    sts = ddrss_load_crypto_key(mc, msg, timeout_us);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_fips_kat_disable_ddrss_load_crypto_key, nullptr, nullptr)
{
    uint32_t mc = 0;
    uint32_t msg = 0;
    uint32_t timeout_us = 0;
    int sts = SILIBS_E_SUPPORT;
    KNG_PLAT_ID platform_id = PLATFORM_FPGA_LARGE;

    // Set up expectations : fips_kat_disable on fpga : DDRSS_HSP_MSG_FIPS_KEYS_LOADED
    msg = DDRSS_HSP_MSG_FIPS_KEYS_LOADED;
    will_return(__wrap_config_get_fips_kat_en, false);
    will_return(__wrap_idsw_get_platform_sdv, platform_id);

    sts = ddrss_load_crypto_key(mc, msg, timeout_us);
    assert_int_equal(sts, SILIBS_E_SUPPORT);

    // Set up expectations : fips_kat_disable on fpga : DDRSS_HSP_MSG_FIPS_KEYS_TEST_COMPLETE
    msg = DDRSS_HSP_MSG_FIPS_KEYS_TEST_COMPLETE;
    will_return(__wrap_config_get_fips_kat_en, false);
    will_return(__wrap_idsw_get_platform_sdv, platform_id);

    sts = ddrss_load_crypto_key(mc, msg, timeout_us);
    assert_int_equal(sts, SILIBS_E_SUPPORT);

    // Set up expectations : fips_kat_disable on fpga : DDRSS_HSP_MSG_PROD_KEYS_LOADED
    msg = DDRSS_HSP_MSG_PROD_KEYS_LOADED;
    will_return(__wrap_config_get_fips_kat_en, false);
    will_return(__wrap_idsw_get_platform_sdv, platform_id);

    expect_value(__wrap_fpfw_icc_base_send_recv_sync, buffer_size, (uint32_t)sizeof(kng_hsp_mailbox_msg));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    sts = ddrss_load_crypto_key(mc, msg, timeout_us);
    assert_int_equal(sts, SILIBS_SUCCESS);

    // Set up expectations : fips_kat_disable on silicon : DDRSS_HSP_MSG_FIPS_KEYS_LOADED
    platform_id = PLATFORM_RVP_EVT_SILICON;
    msg = DDRSS_HSP_MSG_FIPS_KEYS_LOADED;
    will_return(__wrap_config_get_fips_kat_en, false);
    will_return(__wrap_idsw_get_platform_sdv, platform_id);

    sts = ddrss_load_crypto_key(mc, msg, timeout_us);
    assert_int_equal(sts, SILIBS_E_SUPPORT);

    // Set up expectations : fips_kat_disable on silicon : DDRSS_HSP_MSG_FIPS_KEYS_TEST_COMPLETE
    msg = DDRSS_HSP_MSG_FIPS_KEYS_TEST_COMPLETE;
    will_return(__wrap_config_get_fips_kat_en, false);
    will_return(__wrap_idsw_get_platform_sdv, platform_id);

    sts = ddrss_load_crypto_key(mc, msg, timeout_us);
    assert_int_equal(sts, SILIBS_E_SUPPORT);

    // Set up expectations : fips_kat_disable on silicon : DDRSS_HSP_MSG_PROD_KEYS_LOADED
    msg = DDRSS_HSP_MSG_PROD_KEYS_LOADED;
    will_return(__wrap_config_get_fips_kat_en, false);
    will_return(__wrap_idsw_get_platform_sdv, platform_id);

    expect_value(__wrap_fpfw_icc_base_send_recv_sync, buffer_size, (uint32_t)sizeof(kng_hsp_mailbox_msg));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    sts = ddrss_load_crypto_key(mc, msg, timeout_us);
    assert_int_equal(sts, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_ddr_init, nullptr, nullptr)
{
    // Call API under test
    _fpfw_component_ddr.init_fn();
}

} // extern "C"
