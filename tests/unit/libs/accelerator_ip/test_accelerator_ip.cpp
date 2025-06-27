//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_test.cpp
 * Unit test file for accelerator_ip_test lib
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep

extern "C" {
#include "accel_intr.h" // for ACCEL_INTR_RET_FAIL_INTR_INIT, ACCEL_INTR_RET_SUCCESS
#include "accelip_ss_init.h"
#include "atu_lib.h"           // for ATU_ID_MAX, atu_id_t, atu_map_entry_t
#include "kng_soc_constants.h" // for SOC_D0, SDMSS_INSTANCE
#include "silibs_status.h"     // for SILIBS_SUCCESS, SILIBS_E_PARAM

#include <FpFwUtils.h>
#include <accelerator_ip.h> // for scp_accelerators_init, ACCEL_RET_SUCCESS
#include <accelip_id.h>
#include <boot_status.h> // for boot_status_req_t
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h>
#include <fpfw_status.h>
#include <fpfw_timer.h>
#include <fpfw_timer_port.h>
#include <hsp_firmware_headers.h>
#include <idsw_kng.h> // for KNG_DIE_ID
#include <pcr_rpss.h> // for pcr_rpss_entity_t
#include <stdint.h>   // for uintptr_t, uint32_t, uint8_t, uint64_t
#include <stdnoreturn.h>
#include <utils.h> // for UNUSED

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

#define FOUR_MB_SIZE           0x400000
#define BUGCHECK_MOCK_RETURN   (setjmp(mock_jump_buf))
#define bugcheck_mock_return() BUGCHECK_MOCK_RETURN

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

static icc_base_recv_complete_notify s_icc_recv_cb = NULL;
static void* s_icc_recv_ctx = NULL;
static icc_base_send_complete_notify s_icc_send_resp_cb = NULL;
static void* s_icc_send_resp_ctx = NULL;
static kng_hsp_mailbox_msg* mbox_recv_buffs = NULL;
static icc_base_send_complete_notify s_icc_send_cb = NULL;

static uint32_t atu_map_buff[FOUR_MB_SIZE];
static fpfw_icc_base_ctx_t* icc_ctx = nullptr;
static uint32_t dummy_icc_ctx = 0;
static jmp_buf mock_jump_buf;
static bool should_return;

static boot_status_send_complete_notify s_boot_status_send_cb = NULL;
static void* s_boot_status_send_ctx = NULL;

static fpfw_timer_callback s_timer_cb = NULL;

static LargestIntegralType expected_led_status[] = {
    LED_STATUS_CODE_SCP_ACCEL_INIT_START,
    LED_STATUS_CODE_SCP_ACCEL_FAILED,
};

/*--------------------------------- Externs ---------------------------------*/

extern subsystem_ctxt_t* get_accelerator_ctxt(uint32_t* accel_instance_size); // Forward function declaration

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == nullptr)
    {
        return SILIBS_E_PARAM;
    }

    atu_map_entry->mscp_start_address = (uint32_t)atu_map_buff;
    atu_map_entry->mscp_end_address = (uint32_t)(&atu_map_buff[FOUR_MB_SIZE]);

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == nullptr)
    {
        return SILIBS_E_PARAM;
    }

    return mock_type(int);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv()
{
    return mock_type(KNG_PLAT_ID);
}

silibs_status_t __wrap_accelip_ss_init(uintptr_t base_addr, ACCELIP_SS_INSTANCE accelip_id, accelip_ss_init_t* init)
{
    UNUSED(base_addr);
    UNUSED(accelip_id);
    UNUSED(init);

    return mock_type(silibs_status_t);
}

int __wrap_pcr_rpss_init_entity(pcr_rpss_entity_t* pcr, uint16_t enabled_clocks, uintptr_t base)
{
    UNUSED(pcr);
    UNUSED(enabled_clocks);
    UNUSED(base);

    return SILIBS_SUCCESS;
}

int __wrap_pcr_rpss_configure_clock(pcr_rpss_entity_t* pcr, uint32_t us_timeout)
{
    UNUSED(pcr);
    UNUSED(us_timeout);

    return SILIBS_SUCCESS;
}

int __wrap_pcr_rpss_deassert_por_reset(pcr_rpss_entity_t* pcr)
{
    UNUSED(pcr);

    return mock_type(int);
}

void __wrap_debug_print(const char* fmt, ...)
{
    UNUSED(fmt);
}

void __wrap_critical_print(const char* fmt, ...)
{
    UNUSED(fmt);
}

void __wrap_configure_cdedss_hsp_system_addr_map(const uint64_t hsp_base_addr, uintptr_t tower_base_addr)
{
    UNUSED(hsp_base_addr);
    UNUSED(tower_base_addr);
}

void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

void __wrap_configure_cdedss_vab_system_addr_map(CDEDSS_INSTANCE cdedss_id, uintptr_t tower_base_addr)
{
    UNUSED(cdedss_id);
    UNUSED(tower_base_addr);
}

int __wrap_accel_mcp_intr_init(ACCEL_ID accel_type)
{
    if (accel_type >= NUM_VALID_ACCEL_ID)
    {
        return ACCEL_INTR_RET_FAIL_INTR_INIT;
    }

    return mock_type(int);
}

bool __wrap_system_info_is_hsp_present()
{
    return mock_type(bool);
}

bool __wrap_system_info_is_warm_start()
{
    return mock_type(bool);
}

void* __wrap_fpfw_init_get_handle(const char* id)
{
    FPFW_UNUSED(id);

    return mock_ptr_type(void*);
}

fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    assert_non_null(icc_ctx);
    s_icc_send_cb = params->cb;
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    FPFW_UNUSED(params);

    assert_non_null(icc_ctx);
    s_icc_recv_cb = params->cb;
    s_icc_recv_ctx = params->cb_ctx;
    mbox_recv_buffs = (kng_hsp_mailbox_msg*)params->payload_buffer;
    ((kng_hsp_mailbox_msg*)(params->payload_buffer))->header.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP;
    ((kng_hsp_mailbox_msg*)(params->payload_buffer))->rsp.status = FPFW_STATUS_SUCCESS;

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size, size_t* output_recv_bytes)
{
    UNUSED(payload_buffer);
    assert_non_null(icc_ctx);
    FPFW_UNUSED(buffer_size);
    FPFW_UNUSED(output_recv_bytes);
    ((kng_hsp_mailbox_msg*)(payload_buffer))->header.cmd = mock_type(int);

    return mock_type(fpfw_status_t);
}
int __wrap_sdm_init_enable_fence(const uintptr_t ext_cfg_addr, const bool enable)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(enable);

    return mock_type(int);
}

int __wrap_sdm_init_itcm_enable(const uintptr_t ext_cfg_addr, bool enable)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(enable);

    return mock_type(int);
}

int __wrap_sdm_init_deassert_nsysreset(const uintptr_t ext_cfg_addr)
{
    FPFW_UNUSED(ext_cfg_addr);

    return mock_type(int);
}

int __wrap_sdm_init_disable_cpu_wait(const uintptr_t ext_cfg_addr)
{
    FPFW_UNUSED(ext_cfg_addr);

    return mock_type(int);
}

uint32_t __wrap_accel_intr_get_atu_mapped_cfg_address(ACCEL_ID accel_type)
{
    FPFW_UNUSED(accel_type);
    return (uint32_t)atu_map_buff;
}

void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    // Handle noreturn, allowing control to return to test
    if (!should_return)
    {
        longjmp(mock_jump_buf, 1);
    }
}

uint32_t __wrap_atu_svc_accel_atu_addr(ACCEL_ID accel_id)
{
    FPFW_UNUSED(accel_id);

    return mock_type(uint32_t);
}

int32_t __wrap_accel_scp_intr_init(ACCEL_ID accel_type)
{
    FPFW_UNUSED(accel_type);

    return mock_type(int32_t);
}

int __wrap_sdm_init_enable_cpuwait(const uintptr_t ext_cfg_addr)
{
    FPFW_UNUSED(ext_cfg_addr);

    return mock_type(int);
}

int __wrap_sdm_init_assert_nsysreset(const uintptr_t ext_cfg_addr, const bool enable)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(enable);

    return mock_type(int);
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    FPFW_UNUSED(addr);

    return mock_type(int);
}

void __wrap_mmio_update32(volatile uint32_t* addr, uint32_t data, uint32_t mask)
{
    FPFW_UNUSED(addr);
    FPFW_UNUSED(data);
    FPFW_UNUSED(mask);
}

accel_isolation_enable_t __wrap_config_get_sdm_isolation_enable(void)
{
    accel_isolation_enable_t* accel_isolation_enable = mock_ptr_type(accel_isolation_enable_t*);

    return *accel_isolation_enable;
}

accel_isolation_enable_t __wrap_config_get_cded_isolation_enable(void)
{
    accel_isolation_enable_t* accel_isolation_enable = mock_ptr_type(accel_isolation_enable_t*);

    return *accel_isolation_enable;
}

// Setup and callback functions
void cb_fun(void* context)
{
    FPFW_UNUSED(context);
}

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    assert_non_null(p_req_mem);
    assert_true(boot_status == 0);
    check_expected(boot_status_ex);
    s_boot_status_send_cb = p_req_mem->cb;
    s_boot_status_send_ctx = p_req_mem->cb_ctx;
}

void __wrap_post_led_status(boot_status_req_t* p_req_mem, led_status_codes_t status)
{
    assert_non_null(p_req_mem);
    check_expected(status);
}

fpfw_status_t __wrap_fpfw_timer_create(fpfw_timer_t* timer,
                                       fpfw_timer_variant_t variant,
                                       fpfw_dur_t period,
                                       fpfw_timer_callback cb,
                                       void* ctx)
{
    FPFW_UNUSED(ctx);
    FPFW_UNUSED(timer);
    FPFW_UNUSED(cb);
    assert_in_range(variant, FPFW_TIMER_ONESHOT, FPFW_TIMER_PERIODIC);
    assert_in_range(period, 0, UINT64_MAX);
    s_timer_cb = cb;

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_timer_reset(fpfw_timer_t* timer)
{
    FPFW_UNUSED(timer);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_send_resp(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_rsp_t* params)
{
    FPFW_UNUSED(icc_ctx);
    s_icc_send_resp_cb = params->cb;
    s_icc_send_resp_ctx = params->cb_ctx;

    return mock_type(fpfw_status_t);
}
}

TEST_FUNCTION(accelip_pre_boot_config_pass_test, nullptr, nullptr)
{
    DIE_INSTANCE die_id = SOC_D0;

    will_return_always(__wrap_idsw_get_die_id, die_id);

    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {false, false}};
    will_return_always(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);
    will_return_always(__wrap_config_get_cded_isolation_enable, &accel_isolation_enable);

    // Check boot status code
    expect_in_set(__wrap_post_led_status, status, expected_led_status);

    // In init_accelerator()
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0xDEADDEED);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_warm_start, false);
    will_return_always(__wrap_accelip_ss_init, SILIBS_SUCCESS);
    will_return_always(__wrap_accel_scp_intr_init, ACCEL_INTR_RET_SUCCESS);

    assert_int_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(scp_accelerators_init_warm_reset_pass, nullptr, nullptr)
{
    DIE_INSTANCE die_id = SOC_D0;

    will_return_always(__wrap_idsw_get_die_id, die_id);

    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {false, false}};
    will_return_always(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);
    will_return_always(__wrap_config_get_cded_isolation_enable, &accel_isolation_enable);

    // Check boot status code
    expect_in_set(__wrap_post_led_status, status, expected_led_status);

    // In init_accelerator()
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0xDEADDEED);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_warm_start, true);
    will_return_always(__wrap_accel_scp_intr_init, ACCEL_INTR_RET_SUCCESS);

    assert_int_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_pre_boot_config_pass_test_no_hsp, nullptr, nullptr)
{
    DIE_INSTANCE die_id = SOC_D0;

    will_return_always(__wrap_idsw_get_die_id, die_id);

    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {false, false}};
    will_return_always(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);
    will_return_always(__wrap_config_get_cded_isolation_enable, &accel_isolation_enable);

    // Check boot status code
    expect_in_set(__wrap_post_led_status, status, expected_led_status);

    // In init_accelerator()
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0xDEADDEED);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_system_info_is_warm_start, false);
    will_return_always(__wrap_accelip_ss_init, SILIBS_SUCCESS);
    will_return_always(__wrap_accel_scp_intr_init, ACCEL_INTR_RET_SUCCESS);

    assert_int_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_pre_boot_config_isolation, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {true, true}};
    will_return_always(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);
    will_return_always(__wrap_config_get_cded_isolation_enable, &accel_isolation_enable);

    assert_int_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_pre_boot_config_accelip_ss_init_fail_test, nullptr, nullptr)
{
    DIE_INSTANCE die_id = SOC_D0;

    will_return_always(__wrap_idsw_get_die_id, die_id);

    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {false, false}};
    will_return_always(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);

    // Check boot status code ACCEL INIT START
    expect_in_set(__wrap_post_led_status, status, expected_led_status);

    will_return_always(__wrap_atu_svc_accel_atu_addr, 0xDEADDEED);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_warm_start, false);
    will_return_always(__wrap_accelip_ss_init, SILIBS_E_PARAM);

    // Check boot status code ACCEL FAILED
    expect_in_set(__wrap_post_led_status, status, expected_led_status);
    expect_value(__wrap_FpFwAssert, expression, false);

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_pre_boot_config_accelip_ss_init_fail_die1_test, nullptr, nullptr)
{
    DIE_INSTANCE die_id = SOC_D1;

    will_return_always(__wrap_idsw_get_die_id, die_id);

    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {false, false}};
    will_return_always(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);

    // Check boot status code ACCEL INIT START
    expect_in_set(__wrap_post_led_status, status, expected_led_status);

    will_return_always(__wrap_atu_svc_accel_atu_addr, 0xDEADDEED);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_warm_start, false);
    will_return_always(__wrap_accelip_ss_init, SILIBS_E_PARAM);

    // Check boot status code ACCEL FAILED
    expect_in_set(__wrap_post_led_status, status, expected_led_status);
    expect_value(__wrap_FpFwAssert, expression, false);

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(test_scp_accelerators_init_invalid_accel, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0;
    DIE_INSTANCE die_id = SOC_D0;

    will_return_always(__wrap_idsw_get_die_id, die_id);

    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {false, false}};
    will_return_always(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);
    will_return_always(__wrap_config_get_cded_isolation_enable, &accel_isolation_enable);

    // Check boot status code ACCEL INIT START
    expect_in_set(__wrap_post_led_status, status, expected_led_status);

    // Check boot status code ACCEL FAILED
    expect_in_set(__wrap_post_led_status, status, expected_led_status);
    expect_value(__wrap_FpFwAssert, expression, false);

    // Update config with Invalid values
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);
    ACCELIP_SS_INSTANCE old_sdm_d0_type = p_ss_ctxt[0].accelip_metadata.accel_type;
    ACCELIP_SS_INSTANCE old_cded_d0_type = p_ss_ctxt[1].accelip_metadata.accel_type;
    p_ss_ctxt[0].accelip_metadata.accel_type = NUM_ACCELIP_SS_INSTANCES;
    p_ss_ctxt[1].accelip_metadata.accel_type = NUM_ACCELIP_SS_INSTANCES;

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);

    // Update config with original values
    p_ss_ctxt[0].accelip_metadata.accel_type = old_sdm_d0_type;
    p_ss_ctxt[1].accelip_metadata.accel_type = old_cded_d0_type;
}

TEST_FUNCTION(test_scp_accelerators_init_intr_init_failed, nullptr, nullptr)
{
    DIE_INSTANCE die_id = SOC_D0;

    will_return_always(__wrap_idsw_get_die_id, die_id);

    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {false, false}};
    will_return_always(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);

    // Check boot status code ACCEL INIT START
    expect_in_set(__wrap_post_led_status, status, expected_led_status);

    // In init_accelerator()
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0xDEADDEED);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_warm_start, false);
    will_return_always(__wrap_accelip_ss_init, SILIBS_SUCCESS);
    will_return_always(__wrap_accel_scp_intr_init, ACCEL_INTR_RET_FAIL_INTR_NVIC);

    // Check boot status code ACCEL FAILED
    expect_in_set(__wrap_post_led_status, status, expected_led_status);
    expect_value(__wrap_FpFwAssert, expression, false);

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_emcpu_reset_sdm_test, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    will_return(__wrap_idsw_get_die_id, SOC_D0);

    // In emcpu_recovery_sequence()
    will_return(__wrap_sdm_init_enable_cpuwait, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_assert_nsysreset, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0xFFFFFFFF);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_deassert_nsysreset, SILIBS_SUCCESS);

    // In invoke_hsp_accel_fw_download
    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_sdm_init_disable_cpu_wait, SILIBS_SUCCESS);

    accel_core_warm_reset(ACCEL_ID_SDM, cb_fun, NULL, cb_fun, NULL);
    s_icc_recv_cb(&p_ss_ctxt[0], 0, FPFW_STATUS_SUCCESS);
    s_icc_recv_cb(&p_ss_ctxt[0], 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(accelip_emcpu_reset_cded_test, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    will_return(__wrap_idsw_get_die_id, SOC_D0);

    // In emcpu_recovery_sequence()
    will_return(__wrap_sdm_init_enable_cpuwait, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_assert_nsysreset, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0xFFFFFFFF);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_deassert_nsysreset, SILIBS_SUCCESS);

    // In invoke_hsp_accel_fw_download
    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_sdm_init_disable_cpu_wait, SILIBS_SUCCESS);

    accel_core_warm_reset(ACCEL_ID_CDED, cb_fun, NULL, cb_fun, NULL);
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
}

/**
 * Test case to verify that the emcpu reset flow works correctly when the callback function is NULL.
 * This test case is designed to ensure that the accelerator core warm reset can be invoked without
 * a callback function.
 */
TEST_FUNCTION(accelip_emcpu_reset_null_cb_test, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    // Set common expectations of emcpu_recovery_sequence()
    will_return_always(__wrap_sdm_init_enable_cpuwait, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_init_assert_nsysreset, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_init_deassert_nsysreset, SILIBS_SUCCESS);

    // In invoke_hsp_accel_fw_download
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    // Setting up the ICC flow
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return_always(__wrap_sdm_init_disable_cpu_wait, SILIBS_SUCCESS);

    // Set specific expectations of emcpu_recovery_sequence()
    will_return(__wrap_mmio_read32, 0);
    will_return(__wrap_mmio_read32, 0xFFFFFFFF);
    accel_core_warm_reset(ACCEL_ID_CDED, NULL, NULL, cb_fun, NULL);
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);

    // Set specific expectations of emcpu_recovery_sequence()
    will_return(__wrap_mmio_read32, 0);
    will_return(__wrap_mmio_read32, 0xFFFFFFFF);
    accel_core_warm_reset(ACCEL_ID_CDED, cb_fun, NULL, NULL, NULL);
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);

    // Expectations for request_send_complete_cb
    s_icc_send_cb(NULL, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(accelip_emcpu_reset_invalid_state_test, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    // In emcpu_recovery_sequence()
    will_return(__wrap_sdm_init_enable_cpuwait, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_assert_nsysreset, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0xFFFFFFFF);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_deassert_nsysreset, SILIBS_SUCCESS);

    // In invoke_hsp_accel_fw_download
    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_sdm_init_disable_cpu_wait, SILIBS_SUCCESS);

    should_return = false;
    accel_core_warm_reset(ACCEL_ID_CDED, cb_fun, NULL, cb_fun, NULL);

    // Invoke emcpu reset again to trigger failure scenario
    if (!bugcheck_mock_return())
    {
        accel_core_warm_reset(ACCEL_ID_CDED, cb_fun, NULL, cb_fun, NULL);
    }
    else
    {
        // We will hit here when we error out the invalid call - we need to invoke the valid call
        // flow to ensure the rst flow returns to normal
        s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
        s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
    }
}

TEST_FUNCTION(accelip_emcpu_reset_recv_fail_test, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    // In emcpu_recovery_sequence()
    will_return(__wrap_sdm_init_enable_cpuwait, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_assert_nsysreset, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0xFFFFFFFF);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_deassert_nsysreset, SILIBS_SUCCESS);

    // In invoke_hsp_accel_fw_download
    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_DISABLED);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_sdm_init_disable_cpu_wait, SILIBS_SUCCESS);

    should_return = true;

    // The first recv call will fail - BUT WE FORCIBLY CONTINUE EXECUTION
    // IF WE DON'T THE RESET STATE WILL STUCK IN ITCM LOAD PREVENTING FURTHER TESTS
    accel_core_warm_reset(ACCEL_ID_CDED, cb_fun, NULL, cb_fun, NULL);

    // Invoking callback to trigger reset to normal flow
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(accelip_emcpu_reset_send_fail_test, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    // In emcpu_recovery_sequence()
    will_return(__wrap_sdm_init_enable_cpuwait, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_assert_nsysreset, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0xFFFFFFFF);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_deassert_nsysreset, SILIBS_SUCCESS);

    // In invoke_hsp_accel_fw_download
    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_DISABLED);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_DISABLED);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_sdm_init_disable_cpu_wait, SILIBS_SUCCESS);

    should_return = true;

    // The first recv call will fail - BUT WE FORCIBLY CONTINUE EXECUTION
    // IF WE DON'T THE RESET STATE WILL STUCK IN ITCM LOAD PREVENTING FURTHER TESTS
    accel_core_warm_reset(ACCEL_ID_CDED, cb_fun, NULL, cb_fun, NULL);

    // Invoking callback to trigger reset to normal flow
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(accelip_emcpu_reset_send_fail_mbox_cmd_cb, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    // In emcpu_recovery_sequence()
    will_return(__wrap_sdm_init_enable_cpuwait, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_assert_nsysreset, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0xFFFFFFFF);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_deassert_nsysreset, SILIBS_SUCCESS);

    // In invoke_hsp_accel_fw_download
    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_DISABLED);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_DISABLED);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_sdm_init_disable_cpu_wait, SILIBS_SUCCESS);

    should_return = true;

    // The first recv call will fail - BUT WE FORCIBLY CONTINUE EXECUTION
    // IF WE DON'T THE RESET STATE WILL STUCK IN ITCM LOAD PREVENTING FURTHER TESTS
    accel_core_warm_reset(ACCEL_ID_CDED, cb_fun, NULL, cb_fun, NULL);

    // Invoking callback to trigger reset to normal flow
    mbox_recv_buffs->header.cmd = HSP_MAILBOX_CMD_MAX;
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_FAIL);
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(accelip_emcpu_reset_send_fail_mbox_status_cb, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    // In emcpu_recovery_sequence()
    will_return(__wrap_sdm_init_enable_cpuwait, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_assert_nsysreset, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0);
    will_return(__wrap_sdm_init_itcm_enable, SILIBS_SUCCESS);
    will_return(__wrap_mmio_read32, 0xFFFFFFFF);
    will_return(__wrap_sdm_init_enable_fence, SILIBS_SUCCESS);
    will_return(__wrap_sdm_init_deassert_nsysreset, SILIBS_SUCCESS);

    // In invoke_hsp_accel_fw_download
    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_DISABLED);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_DISABLED);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_sdm_init_disable_cpu_wait, SILIBS_SUCCESS);

    should_return = true;

    // The first recv call will fail - BUT WE FORCIBLY CONTINUE EXECUTION
    // IF WE DON'T THE RESET STATE WILL STUCK IN ITCM LOAD PREVENTING FURTHER TESTS
    accel_core_warm_reset(ACCEL_ID_CDED, cb_fun, NULL, cb_fun, NULL);

    // Invoking callback to trigger reset to normal flow
    mbox_recv_buffs->rsp.status = FPFW_STATUS_FAIL;
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_FAIL);
    s_icc_recv_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(accelip_emcpu_reset_send_invalid_accel, nullptr, nullptr)
{
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return(__wrap_idsw_get_die_id, SOC_D0);

    should_return = false;
    if (!bugcheck_mock_return())
    {
        accel_core_warm_reset(NUM_VALID_ACCEL_ID, cb_fun, NULL, cb_fun, NULL);
    }
}

TEST_FUNCTION(accelip_isolation_control_test, nullptr, nullptr)
{
    will_return_count(__wrap_idsw_get_die_id, SOC_D0, 3);
    expect_any_always(__wrap_FpFwAssert, expression);
    will_return_count(__wrap_atu_map, SILIBS_SUCCESS, 2);
    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {false, false}};
    will_return(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);
    will_return(__wrap_config_get_cded_isolation_enable, &accel_isolation_enable);
    will_return_count(__wrap_atu_unmap, SILIBS_SUCCESS, 2);

    assert_int_equal(scp_accelerators_isolation_control(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(mcp_accelerators_init_test_die0_pass, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, SOC_D0);
    expect_value(__wrap_FpFwAssert, expression, true);

    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {false, false}};
    will_return(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);
    will_return(__wrap_config_get_cded_isolation_enable, &accel_isolation_enable);

    // init accelertor
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0xDEADDEED);
    will_return_count(__wrap_accel_mcp_intr_init, ACCEL_INTR_RET_SUCCESS, 2);
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);

    assert_int_equal(mcp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(mcp_accelerators_init_test_die1_pass, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, SOC_D1);
    expect_value(__wrap_FpFwAssert, expression, true);

    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {false, false}};
    will_return(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);
    will_return(__wrap_config_get_cded_isolation_enable, &accel_isolation_enable);

    // init accelertor
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0xDEADDEED);
    will_return_count(__wrap_accel_mcp_intr_init, ACCEL_INTR_RET_SUCCESS, 2);
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);

    assert_int_equal(mcp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(mcp_accelerators_init_test_isolation_pass, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, SOC_D1);
    expect_value(__wrap_FpFwAssert, expression, true);

    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {true, true}};
    will_return(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);
    will_return(__wrap_config_get_cded_isolation_enable, &accel_isolation_enable);

    assert_int_equal(mcp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(mcp_accelerators_init_test_fail1, nullptr, nullptr)
{

    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);
    ACCELIP_SS_INSTANCE og_val0, og_val1;

    og_val0 = p_ss_ctxt[0].accelip_metadata.accel_type;
    og_val1 = p_ss_ctxt[1].accelip_metadata.accel_type;
    p_ss_ctxt[0].accelip_metadata.accel_type = NUM_ACCELIP_SS_INSTANCES;
    p_ss_ctxt[1].accelip_metadata.accel_type = NUM_ACCELIP_SS_INSTANCES;

    will_return_always(__wrap_idsw_get_die_id, SOC_D0);
    expect_value(__wrap_FpFwAssert, expression, true);

    expect_value(__wrap_FpFwAssert, expression, false);
    expect_value(__wrap_FpFwAssert, expression, false);

    assert_int_equal(mcp_accelerators_init(), ACCEL_RET_FAIL_INVALID_PARAMS);

    p_ss_ctxt[0].accelip_metadata.accel_type = og_val0;
    p_ss_ctxt[1].accelip_metadata.accel_type = og_val1;
}

TEST_FUNCTION(mcp_accelerators_init_test_fail3, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, SOC_D0);
    expect_value(__wrap_FpFwAssert, expression, true);

    accel_isolation_enable_t accel_isolation_enable = {.isolation_enable = {false, false}};
    will_return(__wrap_config_get_sdm_isolation_enable, &accel_isolation_enable);
    will_return(__wrap_config_get_cded_isolation_enable, &accel_isolation_enable);

    // init accelertor
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0xDEADDEED);
    will_return_count(__wrap_accel_mcp_intr_init, ACCEL_INTR_RET_FAIL_INTR_INIT, 2);
    expect_value_count(__wrap_FpFwAssert, expression, false, 2);

    assert_int_equal(mcp_accelerators_init(), ACCEL_RET_FAIL_INTR_INIT);
}

TEST_FUNCTION(accel_disable_cpu_wait_test, nullptr, nullptr)
{
    will_return(__wrap_sdm_init_disable_cpu_wait, SILIBS_SUCCESS);

    accel_disable_cpu_wait(ACCEL_ID_SDM);
}

TEST_FUNCTION(accel_disable_cpu_wait_test_fail1, nullptr, nullptr)
{
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return(__wrap_sdm_init_disable_cpu_wait, SILIBS_SUCCESS);

    should_return = true;

    accel_disable_cpu_wait(NUM_VALID_ACCEL_ID);
}

TEST_FUNCTION(accel_get_itcm_addr_sdm_test, nullptr, nullptr)
{
    uint32_t low_addr;
    uint32_t high_addr;

    will_return(__wrap_idsw_get_die_id, SOC_D0);

    accel_get_itcm_addr(ACCEL_ID_SDM, &low_addr, &high_addr);
}

TEST_FUNCTION(accel_get_itcm_addr_cded_test, nullptr, nullptr)
{
    uint32_t low_addr;
    uint32_t high_addr;

    will_return(__wrap_idsw_get_die_id, SOC_D0);

    accel_get_itcm_addr(ACCEL_ID_CDED, &low_addr, &high_addr);
}

TEST_FUNCTION(accel_get_dtcm_addr_sdm_test, nullptr, nullptr)
{
    uint32_t low_addr;
    uint32_t high_addr;

    will_return(__wrap_idsw_get_die_id, SOC_D0);

    accel_get_dtcm_addr(ACCEL_ID_SDM, &low_addr, &high_addr);
}

TEST_FUNCTION(accel_get_dtcm_addr_cded_test, nullptr, nullptr)
{
    uint32_t low_addr;
    uint32_t high_addr;

    will_return(__wrap_idsw_get_die_id, SOC_D0);

    accel_get_dtcm_addr(ACCEL_ID_CDED, &low_addr, &high_addr);
}

TEST_FUNCTION(accel_get_dtcm_addr_test_fail1, nullptr, nullptr)
{
    uint32_t low_addr;
    uint32_t high_addr;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return(__wrap_idsw_get_die_id, NUM_DIE);
    should_return = true;

    accel_get_dtcm_addr(NUM_VALID_ACCEL_ID, &low_addr, &high_addr);
}

TEST_FUNCTION(accel_get_dtcm_addr_test_fail2, nullptr, nullptr)
{
    uint32_t high_addr;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    should_return = false;

    if (!bugcheck_mock_return())
    {
        accel_get_dtcm_addr(ACCEL_ID_CDED, nullptr, &high_addr);
    }
}

TEST_FUNCTION(accel_get_dtcm_addr_test_fail3, nullptr, nullptr)
{
    uint32_t low_addr;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    should_return = false;

    if (!bugcheck_mock_return())
    {
        accel_get_dtcm_addr(ACCEL_ID_CDED, &low_addr, nullptr);
    }
}

TEST_FUNCTION(accel_get_itcm_addr_test_fail1, nullptr, nullptr)
{
    uint32_t low_addr;
    uint32_t high_addr;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return(__wrap_idsw_get_die_id, NUM_DIE);
    should_return = true;

    accel_get_itcm_addr(NUM_VALID_ACCEL_ID, &low_addr, &high_addr);
}

TEST_FUNCTION(accel_get_itcm_addr_test_fail2, nullptr, nullptr)
{
    uint32_t high_addr;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    should_return = false;

    if (!bugcheck_mock_return())
    {
        accel_get_itcm_addr(ACCEL_ID_CDED, nullptr, &high_addr);
    }
}

TEST_FUNCTION(accel_get_itcm_addr_test_fail3, nullptr, nullptr)
{
    uint32_t low_addr;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    should_return = false;

    if (!bugcheck_mock_return())
    {
        accel_get_itcm_addr(ACCEL_ID_CDED, &low_addr, nullptr);
    }
}

/************************* Accel boot status code ************************************/

TEST_FUNCTION(test_accel_setup_boot_status_code_sdm, nullptr, nullptr)
{
    ACCEL_ID accel_id = ACCEL_ID_SDM;

    // Unit test for accel_setup_boot_status_code API for SDM
    will_return_always(__wrap_fpfw_init_get_handle, 0xDEADDEED);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);
    assert_true(accel_setup_boot_status_code(accel_id) == FPFW_STATUS_SUCCESS);

    // Unit test accel_recv_boot_status_msg_icc_cb() for SDM DIE0
    will_return(__wrap_fpfw_timer_reset, FPFW_STATUS_SUCCESS);
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    expect_any(__wrap_boot_status_notify_extd, boot_status_ex);
    s_icc_recv_cb(s_icc_recv_ctx, 0, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send_resp, FPFW_ICC_BASE_STATUS_SUCCESS);
    s_boot_status_send_cb(s_boot_status_send_ctx);

    // Unit test accel_recv_boot_status_msg_icc_cb() for SDM DIE1
    will_return(__wrap_fpfw_timer_reset, FPFW_STATUS_SUCCESS);
    will_return(__wrap_idsw_get_die_id, SOC_D1);
    expect_any(__wrap_boot_status_notify_extd, boot_status_ex);
    s_icc_recv_cb(s_icc_recv_ctx, 0, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send_resp, FPFW_ICC_BASE_STATUS_SUCCESS);
    s_boot_status_send_cb(s_boot_status_send_ctx);

    // Unit test accel_boot_status_timeout_cb() for SDM DIE0
    expect_in_set(__wrap_post_led_status, status, expected_led_status);
    s_timer_cb(NULL, 0);

    // Unit test accel_empty_icc_cb()
    s_icc_send_resp_cb(s_icc_send_resp_ctx, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_accel_setup_boot_status_code_cded, nullptr, nullptr)
{
    ACCEL_ID accel_id = ACCEL_ID_CDED;

    // Unit test for accel_setup_boot_status_code API for CDED
    will_return_always(__wrap_fpfw_init_get_handle, 0xDEADDEED);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);
    assert_true(accel_setup_boot_status_code(accel_id) == FPFW_STATUS_SUCCESS);

    // Unit test accel_recv_boot_status_msg_icc_cb() for CDED DIE0
    will_return(__wrap_fpfw_timer_reset, FPFW_STATUS_SUCCESS);
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    expect_any(__wrap_boot_status_notify_extd, boot_status_ex);
    s_icc_recv_cb(s_icc_recv_ctx, 0, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send_resp, FPFW_ICC_BASE_STATUS_SUCCESS);
    s_boot_status_send_cb(s_boot_status_send_ctx);

    // Unit test accel_recv_boot_status_msg_icc_cb() for CDED DIE1
    will_return(__wrap_fpfw_timer_reset, FPFW_STATUS_SUCCESS);
    will_return(__wrap_idsw_get_die_id, SOC_D1);
    expect_any(__wrap_boot_status_notify_extd, boot_status_ex);
    s_icc_recv_cb(s_icc_recv_ctx, 0, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send_resp, FPFW_ICC_BASE_STATUS_SUCCESS);
    s_boot_status_send_cb(s_boot_status_send_ctx);

    // Unit test accel_boot_status_timeout_cb() for CDED DIE0
    expect_in_set(__wrap_post_led_status, status, expected_led_status);
    s_timer_cb(NULL, 0);

    // Unit test accel_empty_icc_cb()
    s_icc_send_resp_cb(s_icc_send_resp_ctx, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_accel_setup_boot_status_code_invalid_args, nullptr, nullptr)
{
    should_return = false;

    //! Invalid Accel ID
    assert_true(accel_setup_boot_status_code(NUM_VALID_ACCEL_ID) != FPFW_STATUS_SUCCESS);

    //! fpfw_icc_base_recv failed
    will_return_always(__wrap_fpfw_init_get_handle, 0xDEADDEED);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_INVALID_ARGS);
    assert_true(accel_setup_boot_status_code(ACCEL_ID_CDED) != FPFW_STATUS_SUCCESS);

    //! fpfw_timer_create failed
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_timer_create, FPFW_STATUS_INVALID_ARGS);
    assert_true(accel_setup_boot_status_code(ACCEL_ID_CDED) != FPFW_STATUS_SUCCESS);

    //! Setup the ICC recv callbacks
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);
    assert_true(accel_setup_boot_status_code(ACCEL_ID_CDED) == FPFW_STATUS_SUCCESS);

    // Invalid ICC recv callback status
    if (!bugcheck_mock_return())
    {
        will_return(__wrap_fpfw_timer_reset, FPFW_STATUS_SUCCESS);
        expect_any(__wrap_crash_dump_bug_check, errorCode);
        s_icc_recv_cb(s_icc_recv_ctx, 0, FPFW_STATUS_NULL_POINTER);
    }
}