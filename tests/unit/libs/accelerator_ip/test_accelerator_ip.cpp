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
#include <fpfw_icc_base.h>
#include <fpfw_status.h>
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

static icc_base_recv_complete_notify fw_load_cb = NULL;
static uint32_t atu_map_buff[FOUR_MB_SIZE];
static fpfw_icc_base_ctx_t* icc_ctx = nullptr;
static uint32_t dummy_icc_ctx = 0;
static jmp_buf mock_jump_buf;
static bool should_return;

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
    UNUSED(expression);
}

void __wrap_configure_cdedss_vab_system_addr_map(CDEDSS_INSTANCE cdedss_id, uintptr_t tower_base_addr)
{
    UNUSED(cdedss_id);
    UNUSED(tower_base_addr);
}

int __wrap_accel_intr_irq_init(ACCEL_ID accel_type)
{
    if (accel_type >= NUM_VALID_ACCEL_ID)
    {
        return ACCEL_INTR_RET_FAIL_INTR_INIT;
    }

    return ACCEL_INTR_RET_SUCCESS;
}

bool __wrap_system_info_is_hsp_present()
{
    return mock_type(bool);
}

void* __wrap_fpfw_init_get_handle(const char* id)
{
    check_expected_ptr(id);
    function_called();

    return mock_ptr_type(void*);
}

fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    UNUSED(params);
    assert_non_null(icc_ctx);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    assert_non_null(icc_ctx);
    check_expected(params->recv_cmd_code);
    fw_load_cb = params->cb;
    ((kng_hsp_mailbox_msg*)(params->payload_buffer))->header.cmd = mock_type(int);

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

int __wrap__sdm_init_enable_cpuwait(const uintptr_t ext_cfg_addr)
{
    FPFW_UNUSED(ext_cfg_addr);
    return 0x0;
}

int __wrap_sdm_init_enable_fence(const uintptr_t ext_cfg_addr, const bool enable)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(enable);
    return 0x0;
}

int __wrap__sdm_init_assert_nsysreset(const uintptr_t ext_cfg_addr)
{
    FPFW_UNUSED(ext_cfg_addr);
    return 0x0;
}

int __wrap__sdm_init_itcm_enable(const uintptr_t ext_cfg_addr, bool enable)
{
    FPFW_UNUSED(ext_cfg_addr);
    FPFW_UNUSED(enable);
    return 0x0;
}

int __wrap_sdm_init_deassert_nsysreset(const uintptr_t ext_cfg_addr)
{
    FPFW_UNUSED(ext_cfg_addr);
    return 0x0;
}

int __wrap_sdm_init_disable_cpu_wait(const uintptr_t ext_cfg_addr)
{
    FPFW_UNUSED(ext_cfg_addr);
    return 0x0;
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

// Setup and callback functions
void cb_fun(void* context)
{
    FPFW_UNUSED(context);
    printf("IN CALLBACK\n");
}

TEST_FUNCTION(accelip_pre_boot_config_pass_test, nullptr, nullptr)
{
    // Happy case setup
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_system_info_is_hsp_present, false);

    // init accelertor
    will_return_count(__wrap_atu_map, SILIBS_SUCCESS, 3);
    will_return_count(__wrap_accelip_ss_init, SILIBS_SUCCESS, 2);
    will_return_count(__wrap_atu_unmap, SILIBS_SUCCESS, 1);

    assert_int_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_pre_boot_config_atu_map_fail_test, nullptr, nullptr)
{
    // ATU MAP fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_system_info_is_hsp_present, false);

    will_return_count(__wrap_atu_map, SILIBS_E_PARAM, 2);

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_pre_boot_config_atu_unmap_fail_test, nullptr, nullptr)
{
    // ATU UNMAP fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_system_info_is_hsp_present, false);

    will_return_count(__wrap_atu_map, SILIBS_SUCCESS, 2);
    will_return(__wrap_accelip_ss_init, SILIBS_SUCCESS);
    will_return_count(__wrap_atu_unmap, SILIBS_E_PARAM, 1);

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_pre_boot_config_accelip_ss_init_fail_test, nullptr, nullptr)
{
    // Accelip ss init fail
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_system_info_is_hsp_present, false);

    will_return_count(__wrap_atu_map, SILIBS_SUCCESS, 3);
    will_return_count(__wrap_accelip_ss_init, SILIBS_E_PARAM, 2);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    assert_int_not_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_emcpu_reset_cded_test, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    expect_string_count(__wrap_fpfw_init_get_handle, id, "icc_hspmbx", 3);
    expect_function_call_any(__wrap_fpfw_init_get_handle);
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);

    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    will_return_always(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    scp_accelerators_emcpu_reset(ACCEL_ID_SDM, cb_fun, NULL);

    // Invoke the callback mimicking mailbox response - 2 callbacks expected
    fw_load_cb(&p_ss_ctxt[0], 0, FPFW_STATUS_SUCCESS);
    fw_load_cb(&p_ss_ctxt[0], 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(accelip_emcpu_reset_sdm_test, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    expect_string_count(__wrap_fpfw_init_get_handle, id, "icc_hspmbx", 3);
    expect_function_call_any(__wrap_fpfw_init_get_handle);
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);

    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    will_return_always(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    scp_accelerators_emcpu_reset(ACCEL_ID_CDED, cb_fun, NULL);

    // Invoke the callback mimicking mailbox response - 2 callbacks expected
    fw_load_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
    fw_load_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(accelip_emcpu_reset_sdm_no_hsp_test, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    will_return_always(__wrap_system_info_is_hsp_present, false);

    scp_accelerators_emcpu_reset(ACCEL_ID_CDED, cb_fun, NULL);
}

TEST_FUNCTION(accelip_emcpu_reset_invalid_state_test, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    expect_string_count(__wrap_fpfw_init_get_handle, id, "icc_hspmbx", 3);
    expect_function_call_any(__wrap_fpfw_init_get_handle);
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);

    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    will_return_always(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    should_return = false;
    scp_accelerators_emcpu_reset(ACCEL_ID_CDED, cb_fun, NULL);

    // Invoke emcpu reset again to trigger failure scenario
    if (!bugcheck_mock_return())
    {
        scp_accelerators_emcpu_reset(ACCEL_ID_CDED, cb_fun, NULL);
    }
    else
    {
        // We will hit here when we error out the invalid call - we need to invoke the valid call
        // flow to ensure the rst flow returns to normal
        fw_load_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
        fw_load_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
    }
}

TEST_FUNCTION(accelip_emcpu_reset_recv_fail_test, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    expect_string_count(__wrap_fpfw_init_get_handle, id, "icc_hspmbx", 3);
    expect_function_call_any(__wrap_fpfw_init_get_handle);
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);

    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    will_return_always(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_DISABLED);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    should_return = true;

    // The first recv call will fail - BUT WE FORCIBLY CONTINUE EXECUTION
    // IF WE DON'T THE RESET STATE WILL STUCK IN ITCM LOAD PREVENTING FURTHER TESTS
    scp_accelerators_emcpu_reset(ACCEL_ID_CDED, cb_fun, NULL);

    // Invoking callback to trigger reset to normal flow
    fw_load_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
    fw_load_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(accelip_emcpu_reset_send_fail_test, nullptr, nullptr)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    expect_string_count(__wrap_fpfw_init_get_handle, id, "icc_hspmbx", 3);
    expect_function_call_any(__wrap_fpfw_init_get_handle);
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);

    will_return_always(__wrap_idsw_get_die_id, SOC_D0);

    will_return_always(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_DISABLED);

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_DISABLED);

    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    should_return = true;

    // The first recv call will fail - BUT WE FORCIBLY CONTINUE EXECUTION
    // IF WE DON'T THE RESET STATE WILL STUCK IN ITCM LOAD PREVENTING FURTHER TESTS
    scp_accelerators_emcpu_reset(ACCEL_ID_CDED, cb_fun, NULL);

    // Invoking callback to trigger reset to normal flow
    fw_load_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
    fw_load_cb(&p_ss_ctxt[1], 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(accelip_pre_boot_config_cold_boot_pass_test, nullptr, nullptr)
{
    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    expect_string_count(__wrap_fpfw_init_get_handle, id, "icc_hspmbx", 6);
    expect_function_call_any(__wrap_fpfw_init_get_handle);
    will_return_always(__wrap_fpfw_init_get_handle, &icc_ctx);

    will_return_always(__wrap_idsw_get_die_id, SOC_D0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Happy case setup
    // init accelertor
    will_return_count(__wrap_atu_map, SILIBS_SUCCESS, 2);
    will_return_count(__wrap_accelip_ss_init, SILIBS_SUCCESS, 2);

    assert_int_equal(scp_accelerators_init(), ACCEL_RET_SUCCESS);
}

TEST_FUNCTION(accelip_isolation_control_test, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, SOC_D0);
    will_return_count(__wrap_atu_map, SILIBS_SUCCESS, 2);
    will_return_count(__wrap_atu_unmap, SILIBS_SUCCESS, 2);

    assert_int_equal(scp_accelerators_isolation_control(), ACCEL_RET_SUCCESS);
}
}
