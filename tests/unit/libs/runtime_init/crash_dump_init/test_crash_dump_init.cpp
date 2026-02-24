//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_crash_dump_init.cpp
 * Crash dump init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h>
#include <accelip_id.h>
#include <boot_status.h>
#include <crash_dump.h>
#include <crash_dump_dfwk.h>
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#if defined(MCP_RUNTIME_INIT)
    #include <DfwkClient.h>
    #ifndef PLDM_DRV_WORKAROUND
        #include <fpfw_pldm_service.h>
    #endif
    #include <gpio.h>
    #ifdef PLDM_DRV_WORKAROUND
        #include <pldm_drv.h>
    #endif
#endif
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_error.h>
#include <startup_shutdown.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern bool in_memory(uintptr_t start_addr, uintptr_t end_addr);

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_cd_init;
extern fpfw_init_component_t _fpfw_component_cd_mhu_loc;
extern fpfw_init_component_t _fpfw_component_cd_hsp;
extern fpfw_init_component_t _fpfw_component_cd_drv;
#if defined(MCP_RUNTIME_INIT)
extern fpfw_init_component_t _fpfw_component_cd_gpio;
extern fpfw_init_component_t _fpfw_component_cd_pldm;
#endif
#if defined(SCP_RUNTIME_INIT)
extern fpfw_init_component_t _fpfw_component_cd_hsp;
extern fpfw_init_component_t _fpfw_component_cd_mhu_rem;
extern fpfw_init_component_t _fpfw_component_cd_spi_rem;
extern fpfw_init_component_t _fpfw_component_cd_pomesh;
extern fpfw_init_component_t _fpfw_component_cd_accel;
#endif

/*------------- Functions ----------------*/
//
// Mocks
//
crash_dump_context_t* __wrap_crash_dump_context()
{
    return mock_type(crash_dump_context_t*);
}

void __wrap_crash_dump_init(crash_dump_context_t* context)
{
    check_expected_ptr(context);
    check_expected(context->die_index);
    check_expected(context->core_index);
    check_expected(context->is_primary);

    function_called();
}

KNG_STATUS __wrap_crash_dump_register_dump(crash_dump_type_context_t* type_context)
{
    check_expected_ptr(type_context);
    check_expected(type_context->type);

    function_called();

    return mock_type(KNG_STATUS);
}

void __wrap_crash_dump_config_icc(crash_dump_icc_config_t type, fpfw_icc_base_ctx_t*)
{
    check_expected(type);

    function_called();
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    function_called();

    return mock_type(bool);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type()
{
    return mock_type(idsw_cpu_type_t);
}

idsw_plat_id_t __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(idsw_plat_id_t);
}

int32_t __wrap_exception_handler_init(void)
{
    function_called();

    return mock_type(int32_t);
}

uint32_t __wrap_atu_svc_accel_atu_addr(ACCEL_ID accel_id)
{
    FPFW_UNUSED(accel_id);

    return mock_type(uint32_t);
}

bool __wrap_CdRegisterMMIORegisterSet(FPFwCrashDumpCtx* ctx, uint32_t regAddress, uint32_t regCount, uint32_t priority)
{
    assert_non_null(ctx);
    check_expected(regAddress);
    check_expected(regCount);
    check_expected(priority);

    function_called();

    return true;
}

bool __wrap_accel_is_isolation_enabled(ACCEL_ID accel_type)
{
    check_expected(accel_type);

    return mock_type(bool);
}

#if defined(MCP_RUNTIME_INIT)
void __wrap_gpio_interface_init(pgpio_interface_t iface)
{
    assert_non_null(iface);

    function_called();
}

static DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE s_gpio_isr_callback = NULL;

uint32_t __wrap_gpio_register_deferred_isr(pgpio_interface_t iface,
                                           pgpio_request_t request,
                                           uint32_t gpio_ctrl_pin_id,
                                           DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE callback,
                                           void* context)
{
    assert_non_null(iface);
    assert_non_null(request);
    check_expected(gpio_ctrl_pin_id);
    assert_non_null(callback);
    assert_null(context);

    s_gpio_isr_callback = callback;

    function_called();

    return 0;
}

void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    function_called();
}

int32_t __wrap_DfwkClientInterfaceOpen(PDFWK_INTERFACE_HEADER Interface)
{
    assert_non_null(Interface);

    function_called();

    return 0;
}

void __wrap_crash_dump_transfer_full_dump_to_bmc()
{
    function_called();
}

bool __wrap_crash_dump_oob_transfer(void)
{
    return mock_type(bool);
}

void (*__wrap_expiration_function)(ULONG input) = NULL;

UINT __wrap__txe_timer_create(TX_TIMER* timer_ptr,
                              CHAR* name_ptr,
                              VOID (*expiration_function)(ULONG id),
                              ULONG expiration_input,
                              ULONG initial_ticks,
                              ULONG reschedule_ticks,
                              UINT auto_activate,
                              UINT timer_control_block_size)
{
    assert_non_null(timer_ptr);
    assert_non_null(name_ptr);
    assert_non_null(expiration_function);
    FPFW_UNUSED(expiration_input);
    FPFW_UNUSED(initial_ticks);
    FPFW_UNUSED(reschedule_ticks);
    FPFW_UNUSED(auto_activate);
    FPFW_UNUSED(timer_control_block_size);

    __wrap_expiration_function = expiration_function;
    function_called();

    return 0;
}

    #ifdef PLDM_DRV_WORKAROUND
PDFWK_ASYNC_REQUEST_HEADER __wrap_last_async_request_sent = NULL;
DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE __wrap_pldm_platform_event_ready_callback = NULL;
void* __wrap_CompletionContext = NULL;

void __real_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext);

void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext)
{
    __real_DfwkAsyncRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext);

    function_called();
}

fpfw_status_t __wrap_pldm_drv_register_platform_event_ready_notification(pldm_request_t* request)
{
    assert_non_null(request);

    __wrap_pldm_platform_event_ready_callback = request->header.CompletionRoutine;
    __wrap_CompletionContext = request->header.CompletionContext;

    request->header.RequestType = PLDM_GET_READY_ASYNC;
    request->status = FPFW_STATUS_SUCCESS;
    __wrap_last_async_request_sent = &request->header;

    function_called();

    return FPFW_STATUS_SUCCESS;
}
    #else
PlatformEventReadyNotificationCb __wrap_pldm_platform_event_ready_callback = NULL;

fpfw_status_t __wrap_fpfw_pldm_service_register_platform_event_ready_notification(pldm_platform_event_ready_notification* p_notification)
{
    assert_non_null(p_notification);
    assert_non_null(p_notification->CallBack);

    __wrap_pldm_platform_event_ready_callback = p_notification->CallBack;

    function_called();

    return FPFW_STATUS_SUCCESS;
}
    #endif
#endif

void __wrap_crash_dump_device_initialize(pcrash_dump_device_t device, PDFWK_SCHEDULE schedule)
{
    FPFW_UNUSED(schedule);
    assert_non_null(device);

    function_called();
}

void __wrap_crash_dump_interface_initialize(pcrash_dump_interface_t intf, pcrash_dump_device_t device)
{
    assert_non_null(intf);
    assert_non_null(device);

    function_called();
}

int32_t __wrap_sos_register_ssi(PDFWK_INTERFACE_HEADER p_interface,
                                pstartup_ssi_registration_t p_registration,
                                PDFWK_INTERFACE_HEADER p_ssi_interface)
{
    FPFW_UNUSED(p_interface);
    assert_non_null(p_registration);
    assert_non_null(p_ssi_interface);

    function_called();

    return 0;
}

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

//
// Tests
//
TEST_FUNCTION(test_crash_dump_init_in_memory, nullptr, nullptr)
{
    // Test valid address and size
    uintptr_t start_addr = (uintptr_t)0x40;
    uintptr_t end_addr = start_addr + 8 - 1;

    assert_true(in_memory(start_addr, end_addr));

    // Test invalid address
    start_addr = (uintptr_t)0xFFFFFFF0;
    end_addr = start_addr + 6 - 1;

    assert_false(in_memory(start_addr, end_addr));
}

#if defined(SCP_RUNTIME_INIT)
TEST_FUNCTION(test_crash_dump_init_scp, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_idsw_get_die_id, DIE_0);

    expect_any(__wrap_crash_dump_init, context);
    expect_value(__wrap_crash_dump_init, context->die_index, DIE_0);
    expect_value(__wrap_crash_dump_init, context->core_index, CRASH_DUMP_CORE_SCP);
    expect_value(__wrap_crash_dump_init, context->is_primary, false);
    expect_function_call(__wrap_crash_dump_init);

    expect_any(__wrap_crash_dump_register_dump, type_context);
    expect_value(__wrap_crash_dump_register_dump, type_context->type, CRASH_DUMP_TYPE_MINI);
    will_return(__wrap_crash_dump_register_dump, KNG_SUCCESS);
    expect_function_call(__wrap_crash_dump_register_dump);

    will_return(__wrap_exception_handler_init, KNG_SUCCESS);
    expect_function_call(__wrap_exception_handler_init);

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    expect_function_call(__wrap_idhw_is_single_die_boot_en);

    expect_value(__wrap_crash_dump_config_icc, type, CRASH_DUMP_ICC_CONFIG_SPI_REMOTE);
    expect_function_call(__wrap_crash_dump_config_icc);

    expect_value(__wrap_crash_dump_config_icc, type, CRASH_DUMP_ICC_CONFIG_MHU_REMOTE);
    expect_function_call(__wrap_crash_dump_config_icc);

    expect_value(__wrap_crash_dump_config_icc, type, CRASH_DUMP_ICC_CONFIG_HSP);
    expect_function_call(__wrap_crash_dump_config_icc);

    expect_value(__wrap_crash_dump_config_icc, type, CRASH_DUMP_ICC_CONFIG_MHU_LOCAL);
    expect_function_call(__wrap_crash_dump_config_icc);

    // Check dependencies
    assert_string_equal("hw_ver", _fpfw_component_cd_init.children[0]);
    assert_string_equal("gpio_lib", _fpfw_component_cd_init.children[1]);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_CD_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    //  Call API under test
    _fpfw_component_cd_init.init_fn();
}
#endif

#if defined(MCP_RUNTIME_INIT)
TEST_FUNCTION(test_crash_dump_init_mcp, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_idsw_get_platform_sdv, 0x30); // PLATFORM_FPGA

    expect_any(__wrap_crash_dump_init, context);
    expect_value(__wrap_crash_dump_init, context->die_index, DIE_0);
    expect_value(__wrap_crash_dump_init, context->core_index, CRASH_DUMP_CORE_MCP);
    expect_value(__wrap_crash_dump_init, context->is_primary, true);
    expect_function_call(__wrap_crash_dump_init);

    expect_any(__wrap_crash_dump_register_dump, type_context);
    expect_value(__wrap_crash_dump_register_dump, type_context->type, CRASH_DUMP_TYPE_MINI);
    will_return(__wrap_crash_dump_register_dump, KNG_SUCCESS);
    expect_function_call(__wrap_crash_dump_register_dump);

    expect_any(__wrap_crash_dump_register_dump, type_context);
    expect_value(__wrap_crash_dump_register_dump, type_context->type, CRASH_DUMP_TYPE_FULL);
    will_return(__wrap_crash_dump_register_dump, KNG_SUCCESS);
    expect_function_call(__wrap_crash_dump_register_dump);

    will_return(__wrap_exception_handler_init, KNG_SUCCESS);
    expect_function_call(__wrap_exception_handler_init);

    expect_value(__wrap_crash_dump_config_icc, type, CRASH_DUMP_ICC_CONFIG_MHU_LOCAL);
    expect_function_call(__wrap_crash_dump_config_icc);

    // Check dependencies
    assert_string_equal("hw_ver", _fpfw_component_cd_init.children[0]);
    assert_string_equal("gpio_lib", _fpfw_component_cd_init.children[1]);
    assert_string_equal("hw_sem", _fpfw_component_cd_init.children[2]);

    // Call API under test
    _fpfw_component_cd_init.init_fn();
}
#endif

TEST_FUNCTION(test_cd_mhu_loc, nullptr, nullptr)
{
    expect_value(__wrap_crash_dump_config_icc, type, CRASH_DUMP_ICC_CONFIG_MHU_LOCAL);
    expect_function_call(__wrap_crash_dump_config_icc);

    // Check dependencies
    assert_string_equal("cd_init", _fpfw_component_cd_mhu_loc.children[0]);
    assert_string_equal("icc_mscp2mscp", _fpfw_component_cd_mhu_loc.children[1]);

    // Call API under test
    _fpfw_component_cd_mhu_loc.init_fn();
}

#if defined(SCP_RUNTIME_INIT)
TEST_FUNCTION(test_cd_hsp, nullptr, nullptr)
{
    expect_value(__wrap_crash_dump_config_icc, type, CRASH_DUMP_ICC_CONFIG_HSP);
    expect_function_call(__wrap_crash_dump_config_icc);

    // Check dependencies
    assert_string_equal("cd_init", _fpfw_component_cd_hsp.children[0]);
    assert_string_equal("icc_hspmbx", _fpfw_component_cd_hsp.children[1]);

    // Call API under test
    _fpfw_component_cd_hsp.init_fn();
}
#endif

#if defined(MCP_RUNTIME_INIT)
TEST_FUNCTION(test_cd_drv, nullptr, nullptr)
{
    expect_function_call(__wrap_crash_dump_device_initialize);
    expect_function_call(__wrap_crash_dump_interface_initialize);
    expect_function_call(__wrap_sos_register_ssi);

    _fpfw_component_cd_drv.init_fn();
}

TEST_FUNCTION(test_cd_gpio, nullptr, nullptr)
{
    expect_function_call(__wrap_gpio_interface_init);
    expect_function_call(__wrap_DfwkClientInterfaceOpen);
    expect_value(__wrap_gpio_register_deferred_isr, gpio_ctrl_pin_id, GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_6, 4)); // CD_REQUEST
    expect_function_call(__wrap_gpio_register_deferred_isr);

    _fpfw_component_cd_gpio.init_fn();

    expect_value(__wrap_crash_dump_bug_check, errorCode, (uint32_t)KNG_CD_EXTERNAL_REQUEST);
    expect_function_call(__wrap_crash_dump_bug_check);
    s_gpio_isr_callback(NULL, NULL);
}

TEST_FUNCTION(test_cd_bmc, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);

    #ifdef PLDM_DRV_WORKAROUND
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_pldm_drv_register_platform_event_ready_notification);
    // Check dependencies
    assert_string_equal("cd_drv", _fpfw_component_cd_pldm.children[0]);
    assert_string_equal("pldm_drv", _fpfw_component_cd_pldm.children[1]);

    _fpfw_component_cd_pldm.init_fn();

    expect_function_call(__wrap__txe_timer_create);
    __wrap_pldm_platform_event_ready_callback(__wrap_last_async_request_sent, NULL);

    will_return(__wrap_crash_dump_oob_transfer, true);
    expect_function_call(__wrap_crash_dump_transfer_full_dump_to_bmc);
    __wrap_expiration_function(0);

    #else
    expect_function_call(__wrap_fpfw_pldm_service_register_platform_event_ready_notification);
    // Check dependencies
    assert_string_equal("cd_drv", _fpfw_component_cd_pldm.children[0]);
    assert_string_equal("pldm", _fpfw_component_cd_pldm.children[1]);

    _fpfw_component_cd_pldm.init_fn();

    expect_function_call(__wrap__txe_timer_create);
    __wrap_pldm_platform_event_ready_callback(0, NULL);

    will_return(__wrap_crash_dump_oob_transfer, true);
    expect_function_call(__wrap_crash_dump_transfer_full_dump_to_bmc);
    __wrap_expiration_function(0);

    #endif
}
#endif

#if defined(SCP_RUNTIME_INIT)
TEST_FUNCTION(test_cd_drv, nullptr, nullptr)
{
    expect_function_call(__wrap_crash_dump_device_initialize);
    expect_function_call(__wrap_crash_dump_interface_initialize);
    expect_function_call(__wrap_sos_register_ssi);

    _fpfw_component_cd_drv.init_fn();
}

TEST_FUNCTION(test_cd_mhu_rem, nullptr, nullptr)
{
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    expect_function_call(__wrap_idhw_is_single_die_boot_en);
    expect_value(__wrap_crash_dump_config_icc, type, CRASH_DUMP_ICC_CONFIG_MHU_REMOTE);
    expect_function_call(__wrap_crash_dump_config_icc);

    // Check dependencies
    assert_string_equal("cd_init", _fpfw_component_cd_mhu_rem.children[0]);
    assert_string_equal("icc_die2die", _fpfw_component_cd_mhu_rem.children[1]);
    assert_string_equal("hw_ver", _fpfw_component_cd_mhu_rem.children[2]);

    // Call API under test
    _fpfw_component_cd_mhu_rem.init_fn();
}

TEST_FUNCTION(test_cd_spi_rem, nullptr, nullptr)
{
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    expect_function_call(__wrap_idhw_is_single_die_boot_en);
    expect_value(__wrap_crash_dump_config_icc, type, CRASH_DUMP_ICC_CONFIG_SPI_REMOTE);
    expect_function_call(__wrap_crash_dump_config_icc);

    // Check dependencies
    assert_string_equal("cd_init", _fpfw_component_cd_spi_rem.children[0]);
    assert_string_equal("icc_d2dmbx", _fpfw_component_cd_spi_rem.children[1]);
    assert_string_equal("hw_ver", _fpfw_component_cd_spi_rem.children[2]);

    // Call API under test
    _fpfw_component_cd_spi_rem.init_fn();
}

TEST_FUNCTION(test_cd_pomesh, nullptr, nullptr)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP};

    will_return(__wrap_crash_dump_context, &context);
    will_return_always(__wrap_idsw_get_platform_sdv, 0x30); // PLATFORM_FPGA

    expect_any(__wrap_crash_dump_register_dump, type_context);
    expect_value(__wrap_crash_dump_register_dump, type_context->type, CRASH_DUMP_TYPE_FULL);
    will_return(__wrap_crash_dump_register_dump, KNG_SUCCESS);
    expect_function_call(__wrap_crash_dump_register_dump);

    // Check dependencies
    assert_string_equal("cd_init", _fpfw_component_cd_pomesh.children[0]);
    assert_string_equal("ddr", _fpfw_component_cd_pomesh.children[1]);
    assert_string_equal("hw_sem", _fpfw_component_cd_pomesh.children[2]);

    // Call API under test
    _fpfw_component_cd_pomesh.init_fn();
}

TEST_FUNCTION(test_cd_accel, nullptr, nullptr)
{
    // crash_dump_type_context_t full_context = {.type = CRASH_DUMP_TYPE_FULL};
    // crash_dump_context_t context = {.type_ctx = {NULL, &full_context}, .die_index = 0, .core_index = CRASH_DUMP_CORE_SCP};

    // SDM
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_SDM);
    will_return(__wrap_accel_is_isolation_enabled, false);
    expect_value(__wrap_crash_dump_config_icc, type, CRASH_DUMP_ICC_CONFIG_SDM);
    expect_function_call(__wrap_crash_dump_config_icc);

    // will_return_always(__wrap_crash_dump_context, &context);
    // will_return_always(__wrap_atu_svc_accel_atu_addr, 0x12345678);
    // expect_function_call_any(__wrap_CdRegisterMMIORegisterSet);
    // expect_any_always(__wrap_CdRegisterMMIORegisterSet, regAddress);
    // expect_any_always(__wrap_CdRegisterMMIORegisterSet, regCount);
    // expect_any_always(__wrap_CdRegisterMMIORegisterSet, priority);

    // CDED
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_CDED);
    will_return(__wrap_accel_is_isolation_enabled, false);
    expect_value(__wrap_crash_dump_config_icc, type, CRASH_DUMP_ICC_CONFIG_CDED);
    expect_function_call(__wrap_crash_dump_config_icc);

    // Call API under test
    _fpfw_component_cd_accel.init_fn();
}

/**
 * @brief Accel cores are isolated
 *
 */
TEST_FUNCTION(test_cd_accel_isolation, nullptr, nullptr)
{
    // SDM
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_SDM);
    will_return(__wrap_accel_is_isolation_enabled, true);

    // CDED
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_CDED);
    will_return(__wrap_accel_is_isolation_enabled, true);

    // Call API under test
    _fpfw_component_cd_accel.init_fn();
}

#endif // SCP_RUNTIME_INIT
}
