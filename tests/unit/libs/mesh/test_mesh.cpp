//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mesh.cpp
 * mesh tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for expect_value, check_expected, CmockaWra...
#include <nvic.h>

extern "C" {
#include <FPFwInterrupts.h>
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <atu_lib.h>
#include <cml.h>
#include <cmn800.h>
#include <cmn800_error_handler.h> // for acpi_err_sec_generic_t
#include <cmn800_sequence.h>      // for cmn800_sequence_params_t
#include <cmn_config.h>           // for CMN800_CONFIG_CONFIG
#include <d2d_cntr_sync.h>        // for d2d_counter_sync_enable
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_ctx_t
#include <fpfw_status.h>
#include <hsp_firmware_headers.h> // for kng_hsp_mailbox_msg
#include <idsw.h>                 // for idsw_set_platform_sdv, PLATFORM_UN...
#include <idsw_kng.h>             // for KNG_DIE_ID, _KNG_PLAT_ID
#include <interrupts.h>
#include <mesh.h> // for mesh_init
#include <mesh_error_handler.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_exp_spi_synchronize_dies.h> // for mscp_exp_spi_synchronize_dies
#include <ras_arm.h>
#include <stdint.h> // for int32_t, uint32_t
#include <variable_services.h>

extern void mesh_read_cfg_knobs_from_spi(cmn800_sequence_params_t* cmn800_sequence_param);

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
bool simulate_single_die = true;
bool g_test_mesh_d2d_override = false;
uint16_t g_test_d2d_ecc_ce_counter = 0x0;
KNG_DIE_ID g_test_die = (KNG_DIE_ID)0;
static fpfw_icc_base_ctx_t* test_icc_base_hsp_mbx_ctx;
extern ras_agent_entity_t d2dss2_agent[NUM_OF_CCG_WITH_D2D];
silibs_status_t g_ras_arm_agent_set_base = SILIBS_SUCCESS;
static var_service_req_ctx_t s_req_ctx = {};
static var_service_shared_mem_t mem_ctx = {0};
var_service_req_params_t s_set_var_req = {0};
var_service_req_params_t s_get_var_req = {0};
bool memcpy_mock = true;
extern NUMA_CFG numa_cfg;
cmn800_snf_to_mc_config_t mc_config = {
    .is_numa_enabled = 1,
    .ddr_mc_map = {0, 1, 4, 2, 3, 6, 5, 8, 9, 7, 10, 11, 23, 22, 19, 21, 20, 17, 18, 15, 14, 16, 13, 12},
    .map_size = 24,
    .hash_select = 0,
    .i3c_dimm_sku = 0,
    .i3c_ddrss_mask = 0};

d2d_cfg_t unit_test_default_d2d_cfg = {
    .d2d_pll_divder = 0x2,
    .d2d_ref_divder = 0x2,
    .d2d_pll_fb_devider = 0x14,
    .d2d_ecc_cfg = 0,
    .d2d_tx_interface_clk_alignment = 0,
    .d2d_ras_enable = 0,
    .d2d_sleep_cfg = 1,
    .d2d_sleep_cfg_entry = 0,
    .d2d_rxcal_find_goodlanes_skip = 1,
};

ccg_cfg_t unit_test_default_ccg_cfg = {.por_ccg_ha_aux_ctl = 0x3C0008ULL,
                                       .por_ccg_ha_cfg_ctl = 0x40040ULL,
                                       .por_ccg_ha_cxprtcl_link0_ctl = 0x1C0000ULL,
                                       .por_ccg_ra_cfg_ctl = 0x7C1007ULL,
                                       .por_ccg_ra_aux_ctl = 0x1B1F343CC3846ULL,
                                       .por_ccg_ra_ccprtcl_link0_ctl = 0x2C00000ULL,
                                       .por_ccg_ra_cbusy_limit_ctl = 0x181008ULL,
                                       .por_ccla_aux_ctl = 0x1220000000004ULL};

/*------------- Functions ----------------*/
//! Mocks for mailbox primitives called inside hsp_send_recv_enable_smmu()
fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size, size_t* output_recv_bytes)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(buffer_size);

    kng_hsp_mailbox_msg* msg = (kng_hsp_mailbox_msg*)payload_buffer;
    msg->header.cmd += 0x1; // REQ/RSP messages is in Pair in the kingsgate_hsp_mailbox_commands.h file
    msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
    *output_recv_bytes = sizeof(kng_hsp_mailbox_msg);

    return FPFW_ICC_BASE_STATUS_SUCCESS;
}

fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(payload_buffer);
    FPFW_UNUSED(buffer_size);

    return FPFW_ICC_BASE_STATUS_SUCCESS;
}

bool __wrap_system_info_is_hsp_present()
{
    return true;
}

bool __wrap_system_info_is_warm_start()
{
    return mock_type(bool);
}

void __wrap_mmio_write32(void* addr, uint32_t data)
{
    function_called();
    FPFW_UNUSED(addr);
    FPFW_UNUSED(data);
    return;
}

uint32_t __wrap_mmio_read32(void* addr)
{
    FPFW_UNUSED(addr);
    return 0;
}

int __wrap_mscp_exp_spi_synchronize_dies(mscp_exp_spi_sync_point_t sync_point, int die_id)
{
    FPFW_UNUSED(die_id); //! unused parameter
    assert_int_equal(sync_point.value, D2D_NUMA_INFO);
    return mock_type(int);
}

int32_t __wrap_variable_service_initialize_ctx(var_service_req_ctx_t* var_serv_ctx, var_service_shared_mem_t* mem_ctx)
{
    check_expected(var_serv_ctx);
    check_expected(mem_ctx);
    return mock_type(int);
}

void __wrap_variable_service_sync_set_variable(var_service_req_ctx_t* var_serv_ctx, var_service_req_params_t* req_params)
{
    check_expected(var_serv_ctx);
    check_expected(req_params);
    function_called();
}

int32_t __wrap_variable_service_sync_get_variable(var_service_req_ctx_t* var_serv_ctx, var_service_req_params_t* req_params)
{
    check_expected(var_serv_ctx);
    check_expected(req_params);
    function_called();
    return mock_type(int);
}

void* __real_memset(void* str, int c, size_t n);

void* __wrap_memset(void* str, int c, size_t n)
{
    if (memcpy_mock)
    {
        return NULL;
    }
    return __real_memset(str, c, n);
}

void* __real_memcmp(void* __a, const void* __b, size_t __c);

void* __wrap_memcmp(void* __a, const void* __b, size_t __c)
{
    if (memcpy_mock)
    {
        return 0;
    }
    return __real_memcmp(__a, __b, __c);
}
//
// Setup and Teardown Functions
//
static int setup_svp_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_SVP_SIM);
    simulate_single_die = true;
    return 0;
}

static int setup_svp_platform_dual_die(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_SVP_SIM);
    simulate_single_die = false;
    return 0;
}

static int setup_fpga_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE);
    simulate_single_die = true;
    return 0;
}

static int setup_fpga_platform_dual_die(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE);
    simulate_single_die = false;
    return 0;
}

static int setup_undefined_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_UNDEFINED);
    simulate_single_die = true;
    g_test_die = (KNG_DIE_ID)0;
    g_ras_arm_agent_set_base = SILIBS_SUCCESS;
    g_test_mesh_d2d_override = false;
    g_test_d2d_ecc_ce_counter = 0x0;
    return 0;
}

//
// Mocks
//
int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }

    // Keep mscp base non-zero to allow checking base address in UTs
    atu_map_entry->mscp_start_address = 0xffffffff;

    return 0;
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }

    return 0;
}

int __wrap_atu_translate_address(atu_id_t atu_id, uint64_t ap_addr, uint32_t* mscp_addr)
{
    if (atu_id >= ATU_ID_MAX)
    {
        return SILIBS_E_PARAM;
    }
    assert_non_null(mscp_addr);
    FPFW_UNUSED(ap_addr);

    *mscp_addr = 0xffffffff;
    return 0;
}

int __wrap_cmn800_sequence(cmn800_sequence_params_t cmn800_sequence_param)
{
    check_expected(cmn800_sequence_param.die_num);
    check_expected(cmn800_sequence_param.cmn_config_enum);
    check_expected(cmn800_sequence_param.CMN_INIT);
    check_expected(cmn800_sequence_param.BOOT_2D_ENABLE);
    check_expected(cmn800_sequence_param.HNS_SPARE_DIE0);
    check_expected(cmn800_sequence_param.HNS_SPARE_DIE1);
    check_expected(cmn800_sequence_param.D2D_INIT);
    function_called();
    return 0;
}

int __wrap_d2dss_sequence(cmn800_sequence_params_t cmn800_sequence_param)
{
    check_expected(cmn800_sequence_param.die_num);
    check_expected(cmn800_sequence_param.cmn_config_enum);
    check_expected(cmn800_sequence_param.CMN_INIT);
    check_expected(cmn800_sequence_param.BOOT_2D_ENABLE);
    check_expected(cmn800_sequence_param.HNS_SPARE_DIE0);
    check_expected(cmn800_sequence_param.HNS_SPARE_DIE1);
    check_expected(cmn800_sequence_param.D2D_INIT);
    function_called();
    return 0;
}

int __wrap_cmn800_sequence_svp_updates(cmn800_sequence_params_t cmn800_sequence_param)
{
    check_expected(cmn800_sequence_param.die_num);

    function_called();
    return 0;
}
bool __wrap_idhw_is_single_die_boot_en(void)
{
    return simulate_single_die;
}

KNG_DIE_ID __wrap_idhw_get_die_id(void)
{
    return g_test_die;
}

nvic_status_t __wrap_nvic_irq_set_isr_with_param(uint32_t irq_num, isr_callback_fn_with_params_t isr, void* parameter)
{
    check_expected(irq_num);
    check_expected(isr);
    FPFW_UNUSED(parameter);

    return (NVIC_STATUS_SUCCESS);
}

nvic_status_t __wrap_nvic_irq_clear_pending(uint32_t irq_num)
{
    check_expected(irq_num);
    return (NVIC_STATUS_SUCCESS);
}

nvic_status_t __wrap_nvic_irq_enable(uint32_t irq_num)
{
    check_expected(irq_num);
    return (NVIC_STATUS_SUCCESS);
}

void __wrap_interrupt_handler_mesh_ras_error(acpi_err_sec_generic_t* mesh_cper, bool fault, bool non_secure, uint8_t die_num)
{
    assert_non_null(mesh_cper);
    check_expected(fault);
    check_expected(non_secure);
    check_expected(die_num);
    function_called();
}

void __wrap_ras_arm_agent_setup_entity(ras_agent_entity_t* agent)
{
    check_expected(agent);
    function_called();
}

silibs_status_t __wrap_ras_arm_agent_init(ras_agent_entity_t* agent, uintptr_t base, const char* name)
{
    check_expected(agent);
    assert_non_null(base);
    assert_non_null(name);
    function_called();
    return SILIBS_SUCCESS;
}

silibs_status_t __wrap_ras_arm_agent_enable_signaling_by_type(ras_agent_entity_t* agent, uint64_t types)
{
    check_expected(agent);
    FPFW_UNUSED(types);
    function_called();
    return SILIBS_SUCCESS;
}

silibs_status_t __wrap_ras_arm_agent_enable_fhi(ras_agent_entity_t* agent)
{
    check_expected(agent);
    function_called();
    return SILIBS_SUCCESS;
}

silibs_status_t __wrap_ras_arm_agent_enable_logging(ras_agent_entity_t* agent)
{
    check_expected(agent);
    function_called();
    return SILIBS_SUCCESS;
}

silibs_status_t __wrap_ras_arm_agent_set_base(ras_agent_entity_t* agent, uintptr_t base)
{
    check_expected(agent);
    assert_non_null(base);
    function_called();
    return g_ras_arm_agent_set_base;
}

silibs_status_t ras_wrap_handler(ras_error_record_t* record)
{
    assert_non_null(record);
    return SILIBS_SUCCESS;
}

bool __wrap_ras_arm_agent_probe(ras_agent_entity_t* agent, ras_error_record_t* record)
{
    check_expected(agent);
    assert_non_null(record);
    record->handler = mock_type(ras_generic_handler_t);
    record->err_code = mock_type(uint32_t);
    record->err_code_valid = mock_type(bool);
    function_called();
    return mock_type(bool);
}

int __wrap_ras_print_record(ras_error_record_t* record)
{
    FPFW_UNUSED(record);
    function_called();
    return SILIBS_SUCCESS;
}
silibs_status_t __wrap_ras_arm_agent_trigger_by_type(ras_agent_entity_t* agent, uint32_t types)
{
    check_expected(agent);
    FPFW_UNUSED(types);
    function_called();
    return SILIBS_SUCCESS;
}

void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    check_expected(p1);
    check_expected(p2);
    check_expected(p3);
    check_expected(p4);
    function_called();
}

void __wrap_d2d_cntr_sync_enable(void)
{
    function_called();
}

cmn800_snf_to_mc_config_t* __wrap_cmn800_generate_ddr_mc_map_from_snf_config(const config_t* config)
{
    FPFW_UNUSED(config);
    function_called();
    return &mc_config;
}

void setup_common_isr_expectations(void)
{
    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_INTREQFAULTS);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)mesh_fault_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_INTREQFAULTS);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_INTREQFAULTS);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_INTREQERRS);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)mesh_error_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_INTREQERRS);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_INTREQERRS);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_INTREQFAULTNS);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)mesh_ns_fault_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_INTREQFAULTNS);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_INTREQFAULTNS);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_INTREQERRNS);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, (FPFwCoreInterruptHandler)mesh_ns_error_isr);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_INTREQERRNS);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_INTREQERRNS);
}

uint16_t __wrap_config_get_d2d_ecc_ce_counter(void)
{
    return g_test_d2d_ecc_ce_counter;
}

bool __wrap_config_get_mesh_d2d_override(void)
{
    return g_test_mesh_d2d_override;
}

cmn800_sam_cfg_t* __wrap_cmn800_get_mesh_sam_cfg_knob(void)
{
    function_called();
    return &default_sam_cfg_knb; // This is in SiLibs
}

cmn800_ras_cfg_t* __wrap_cmn800_get_mesh_ras_cfg_knob(void)
{
    function_called();
    return &default_mesh_ras_cfg_knb; // This is in SiLibs
}

ccg_cfg_t* __wrap_get_ccg_knob_defaults(void)
{
    function_called();
    return &unit_test_default_ccg_cfg;
}

d2d_cfg_t* __wrap_get_default_d2d_cfg(void)
{
    function_called();
    return &unit_test_default_d2d_cfg;
}

int __wrap_process_mesh_binary_from_spi(uintptr_t pMeshBinHeader, uint32_t config_enum)
{
    assert_non_null(pMeshBinHeader);
    FPFW_UNUSED(config_enum);
    function_called();
    return 0;
}

void setup_common_numa_variables_expectations(KNG_DIE_ID die_num)
{
    if (die_num == 0)
    {
        mem_ctx.payload_base = (uintptr_t)SCP_EXP_SCP_MESH_NUMA_VARIABLE_SERVICE_PAYLOAD_BASE;
        mem_ctx.max_payload_size = SCP_EXP_SCP_MESH_NUMA_VARIABLE_SERVICE_PAYLOAD_SIZE;
        expect_memory(__wrap_variable_service_initialize_ctx, var_serv_ctx, &s_req_ctx, sizeof(s_req_ctx));
        expect_memory(__wrap_variable_service_initialize_ctx, mem_ctx, &mem_ctx, sizeof(mem_ctx));
        will_return(__wrap_variable_service_initialize_ctx, 0);

        expect_any(__wrap_variable_service_sync_set_variable, var_serv_ctx);
        expect_any(__wrap_variable_service_sync_set_variable, req_params);
        expect_function_call(__wrap_variable_service_sync_set_variable);

        expect_any(__wrap_variable_service_sync_get_variable, var_serv_ctx);
        expect_any(__wrap_variable_service_sync_get_variable, req_params);
        expect_function_call(__wrap_variable_service_sync_get_variable);
        will_return(__wrap_variable_service_sync_get_variable, 0);
    }
}
//
// Tests
//
// Single Die Boot
TEST_FUNCTION(test_mesh_init_single_die_boot_Die_0_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_system_info_is_warm_start, false);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_1D_UMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    // Setup the common ISR Expectations
    setup_common_isr_expectations();

    setup_common_numa_variables_expectations(test_die);
    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

TEST_FUNCTION(test_mesh_init_single_die_boot_Die_1_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    will_return(__wrap_system_info_is_warm_start, false);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_1D_UMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    // Setup the common ISR Expectations
    setup_common_isr_expectations();

    setup_common_numa_variables_expectations(test_die);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

TEST_FUNCTION(test_mesh_init_single_die_boot_Die_0_FPGA, setup_fpga_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_system_info_is_warm_start, false);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_1D_UMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    // Setup the common ISR Expectations
    setup_common_isr_expectations();

    setup_common_numa_variables_expectations(test_die);
    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

TEST_FUNCTION(test_mesh_init_single_die_boot_Die_1_FPGA, setup_fpga_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    will_return(__wrap_system_info_is_warm_start, false);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_1D_UMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    // Setup the common ISR Expectations
    setup_common_isr_expectations();
    setup_common_numa_variables_expectations(test_die);
    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

// Dual Die Boot
TEST_FUNCTION(test_mesh_init_dual_die_boot_Die_0_SVP, setup_svp_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_system_info_is_warm_start, false);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_d2dss_sequence);

    // Setup the common ISR Expectations
    setup_common_isr_expectations();
    // d2d_ras_init
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_setup_entity, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_setup_entity);
        expect_value(__wrap_ras_arm_agent_init, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_init);
        expect_value(__wrap_ras_arm_agent_enable_signaling_by_type, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_signaling_by_type);
        expect_value(__wrap_ras_arm_agent_enable_fhi, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_fhi);
        expect_value(__wrap_ras_arm_agent_enable_logging, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_logging);
    }
    setup_common_numa_variables_expectations(test_die);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

TEST_FUNCTION(test_mesh_init_dual_die_boot_Die_1_SVP, setup_svp_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    will_return(__wrap_system_info_is_warm_start, false);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_d2dss_sequence);

    // Setup the common ISR Expectations
    setup_common_isr_expectations();

    // d2d_ras_init
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_setup_entity, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_setup_entity);
        expect_value(__wrap_ras_arm_agent_init, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_init);
        expect_value(__wrap_ras_arm_agent_enable_signaling_by_type, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_signaling_by_type);
        expect_value(__wrap_ras_arm_agent_enable_fhi, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_fhi);
        expect_value(__wrap_ras_arm_agent_enable_logging, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_logging);
    }

    setup_common_numa_variables_expectations(test_die);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);
    expect_function_call(__wrap_d2d_cntr_sync_enable);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

TEST_FUNCTION(test_mesh_init_dual_die_boot_Die_0_FPGA, setup_fpga_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_system_info_is_warm_start, false);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_d2dss_sequence);

    // Setup the common ISR Expectations
    setup_common_isr_expectations();

    // d2d_ras_init
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_setup_entity, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_setup_entity);
        expect_value(__wrap_ras_arm_agent_init, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_init);
        expect_value(__wrap_ras_arm_agent_enable_signaling_by_type, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_signaling_by_type);
        expect_value(__wrap_ras_arm_agent_enable_fhi, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_fhi);
        expect_value(__wrap_ras_arm_agent_enable_logging, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_logging);
    }

    setup_common_numa_variables_expectations(test_die);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

TEST_FUNCTION(test_mesh_init_dual_die_boot_Die_1_FPGA, setup_fpga_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    will_return(__wrap_system_info_is_warm_start, false);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_d2dss_sequence);

    // Setup the common ISR Expectations
    setup_common_isr_expectations();

    // d2d_ras_init
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_setup_entity, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_setup_entity);
        expect_value(__wrap_ras_arm_agent_init, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_init);
        expect_value(__wrap_ras_arm_agent_enable_signaling_by_type, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_signaling_by_type);
        expect_value(__wrap_ras_arm_agent_enable_fhi, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_fhi);
        expect_value(__wrap_ras_arm_agent_enable_logging, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_logging);
    }

    setup_common_numa_variables_expectations(test_die);
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);
    expect_function_call(__wrap_d2d_cntr_sync_enable);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

// Mesh Error ISR Die 0
TEST_FUNCTION(test_mesh_error_handler_mesh_error_isr_Die_0_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    // Call API under test
    mesh_error_isr(NULL);
}

// Mesh Error ISR Die 1
TEST_FUNCTION(test_mesh_error_handler_mesh_error_isr_Die_1_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    // Call API under test
    mesh_error_isr(NULL);
}

// Mesh Fault ISR Die 0
TEST_FUNCTION(test_mesh_error_handler_mesh_fault_isr_Die_0_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    // Call API under test
    mesh_fault_isr(NULL);
}

// Mesh Fault ISR Die 1
TEST_FUNCTION(test_mesh_error_handler_mesh_fault_isr_Die_1_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    // Call API under test
    mesh_fault_isr(NULL);
}

// Mesh NS Error ISR Die 0
TEST_FUNCTION(test_mesh_error_handler_mesh_ns_error_isr_Die_0_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    // Call API under test
    mesh_ns_error_isr(NULL);
}

// Mesh NS Error ISR Die 1
TEST_FUNCTION(test_mesh_error_handler_mesh_ns_error_isr_Die_1_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    // Call API under test
    mesh_ns_error_isr(NULL);
}

// Mesh NS Fault ISR Die 0
TEST_FUNCTION(test_mesh_error_handler_mesh_ns_fault_isr_Die_0_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    // Call API under test
    mesh_ns_fault_isr(NULL);
}

// Mesh NS Fault ISR Die 1
TEST_FUNCTION(test_mesh_error_handler_mesh_ns_fault_isr_Die_1_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    // Call API under test
    mesh_ns_fault_isr(NULL);
}

// D2D RAS Error Inj
TEST_FUNCTION(test_mesh_error_handler_d2d_ras_error_inj, setup_svp_platform, setup_undefined_platform)
{
    uint32_t err_inj = D2DSS_TEST_RAS_ERROR_INF;
    uint32_t err_cnt_down = D2DSS_TEST_RAS_INJ_COUNTER;

    // Set up expectations
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_set_base, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_set_base);
        expect_value(__wrap_ras_arm_agent_trigger_by_type, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_trigger_by_type);

        // Call API under test
        d2d_ras_error_inj(d2d_subsystem, err_inj, err_cnt_down);
    }
}

// D2D RAS Error Inj 2
TEST_FUNCTION(test_mesh_error_handler_d2d_ras_error_inj_2, setup_svp_platform, setup_undefined_platform)
{
    uint32_t err_inj = D2DSS_TEST_RAS_ERROR_INF;
    uint32_t err_cnt_down = D2DSS_TEST_RAS_INJ_COUNTER;

    // Set up expectations
    g_ras_arm_agent_set_base = SILIBS_E_PARAM;
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_set_base, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_set_base);

        // Call API under test
        d2d_ras_error_inj(d2d_subsystem, err_inj, err_cnt_down);
    }
}

// D2D RAS Error Inj 3 - Bad D2D Subsystem
TEST_FUNCTION(test_mesh_error_handler_d2d_ras_error_inj_3, setup_svp_platform, setup_undefined_platform)
{
    uint32_t err_inj = D2DSS_TEST_RAS_ERROR_INF;
    uint32_t err_cnt_down = D2DSS_TEST_RAS_INJ_COUNTER;

    // Set up expectations
    uint8_t d2d_subsystem = NUM_OF_CCG_WITH_D2D;

    // Call API under test
    d2d_ras_error_inj(d2d_subsystem, err_inj, err_cnt_down);
}

// D2D RAS Error ISR
TEST_FUNCTION(test_mesh_error_handler_d2d_ras_error_isr, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_set_base, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_set_base);
        expect_value(__wrap_ras_arm_agent_probe, agent, &d2dss2_agent[d2d_subsystem]);
        will_return(__wrap_ras_arm_agent_probe, NULL); // handler
        will_return(__wrap_ras_arm_agent_probe, 0x0);  // error code
        will_return(__wrap_ras_arm_agent_probe, true); // error code valid
        will_return(__wrap_ras_arm_agent_probe, true); // return
        expect_function_call(__wrap_ras_arm_agent_probe);
        expect_function_call(__wrap_ras_print_record);
    }
    // Call API under test
    d2d_error_isr(NULL);
}

TEST_FUNCTION(test_mesh_error_handler_d2d_ras_error_isr_crash, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    expect_value(__wrap_ras_arm_agent_set_base, agent, &d2dss2_agent[0]);
    expect_function_call(__wrap_ras_arm_agent_set_base);
    expect_value(__wrap_ras_arm_agent_probe, agent, &d2dss2_agent[0]);
    will_return(__wrap_ras_arm_agent_probe, NULL);       // handler
    will_return(__wrap_ras_arm_agent_probe, 0x80000001); // error code
    will_return(__wrap_ras_arm_agent_probe, true);       // error code valid
    will_return(__wrap_ras_arm_agent_probe, true);       // return
    expect_function_call(__wrap_ras_arm_agent_probe);
    expect_function_call(__wrap_ras_print_record);

    expect_value(__wrap_ras_arm_agent_set_base, agent, &d2dss2_agent[1]);
    expect_function_call(__wrap_ras_arm_agent_set_base);
    expect_value(__wrap_ras_arm_agent_probe, agent, &d2dss2_agent[1]);
    will_return(__wrap_ras_arm_agent_probe, ras_wrap_handler); // handler
    will_return(__wrap_ras_arm_agent_probe, 0x0);              // error code
    will_return(__wrap_ras_arm_agent_probe, false);            // error code valid
    will_return(__wrap_ras_arm_agent_probe, false);            // return
    expect_function_call(__wrap_ras_arm_agent_probe);

    expect_value(__wrap_ras_arm_agent_set_base, agent, &d2dss2_agent[2]);
    expect_function_call(__wrap_ras_arm_agent_set_base);
    expect_value(__wrap_ras_arm_agent_probe, agent, &d2dss2_agent[2]);
    will_return(__wrap_ras_arm_agent_probe, ras_wrap_handler); // handler
    will_return(__wrap_ras_arm_agent_probe, 0x80000001);       // error code
    will_return(__wrap_ras_arm_agent_probe, true);             // error code valid
    will_return(__wrap_ras_arm_agent_probe, true);             // return
    expect_function_call(__wrap_ras_arm_agent_probe);
    expect_function_call(__wrap_ras_print_record);

    expect_value(__wrap_crash_dump_bug_check, errorCode, 0x80000001);
    expect_any(__wrap_crash_dump_bug_check, p1);
    expect_any(__wrap_crash_dump_bug_check, p2);
    expect_any(__wrap_crash_dump_bug_check, p3);
    expect_any(__wrap_crash_dump_bug_check, p4);
    expect_function_call(__wrap_crash_dump_bug_check);

    // Call API under test
    d2d_error_isr(NULL);
}

// D2D RAS Init for Die 1
TEST_FUNCTION(test_mesh_error_handler_d2d_ras_init_Die_1, setup_svp_platform, setup_undefined_platform)
{
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    // d2d_ras_init
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_setup_entity, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_setup_entity);
        expect_value(__wrap_ras_arm_agent_init, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_init);
        expect_value(__wrap_ras_arm_agent_enable_signaling_by_type, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_signaling_by_type);
        expect_value(__wrap_ras_arm_agent_enable_fhi, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_fhi);
        expect_value(__wrap_ras_arm_agent_enable_logging, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_logging);
    }
    // Call API under test
    d2d_ras_init();
}

// D2D RAS Init for Die 0
TEST_FUNCTION(test_mesh_error_handler_d2d_ras_init_Die_0, setup_svp_platform, setup_undefined_platform)
{
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    // d2d_ras_init
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_setup_entity, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_setup_entity);
        expect_value(__wrap_ras_arm_agent_init, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_init);
        expect_value(__wrap_ras_arm_agent_enable_signaling_by_type, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_signaling_by_type);
        expect_value(__wrap_ras_arm_agent_enable_fhi, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_fhi);
        expect_value(__wrap_ras_arm_agent_enable_logging, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_logging);
        expect_function_call(__wrap_mmio_write32);
    }
    // Override the g_test_d2d_ecc_ce_counter to test the D2D RAS init
    g_test_d2d_ecc_ce_counter = 0x30;

    // Call API under test
    d2d_ras_init();
}

void verify_mesh_config_knobs(void)
{
    // Mesh Config Knobs
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cbusy_limit_ctl, 0x302010);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cbusy_resp_ctl, 0x180400800040000);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cbusy_sn_ctl, 0x100001000200040);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cbusy_write_limit_ctl, 0x302010);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cbusy_mode_ctl, 0x3000);

    assert_int_equal(default_sam_cfg_knb.mesh_hnf_aux_ctl, 0x2000001000200002);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_aux_ctl_1, 0x3001005A900);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cfg_ctl, 0x2000C017389A3000);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_lbt_cfg_ctl, 0x7F7F00);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_lbt_aux_ctl, 0x440000000000006);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_lbt_cbusy_ctl, 0x0);

    assert_int_equal(default_sam_cfg_knb.mesh_hni_cfg_ctl[0], 0x1);
    assert_int_equal(default_sam_cfg_knb.mesh_hni_cfg_ctl[1], 0x1);
    assert_int_equal(default_sam_cfg_knb.mesh_hni_cfg_ctl[2], 0x1);
    assert_int_equal(default_sam_cfg_knb.mesh_hni_cfg_ctl[3], 0x1);
    assert_int_equal(default_sam_cfg_knb.mesh_hni_cfg_ctl[4], 0x1);
    assert_int_equal(default_sam_cfg_knb.mesh_hni_cfg_ctl[5], 0x1);

    assert_int_equal(default_sam_cfg_knb.mesh_hni_aux_ctl[0], 0x1000);
    assert_int_equal(default_sam_cfg_knb.mesh_hni_aux_ctl[1], 0x1000);
    assert_int_equal(default_sam_cfg_knb.mesh_hni_aux_ctl[2], 0x1000);
    assert_int_equal(default_sam_cfg_knb.mesh_hni_aux_ctl[3], 0x1000);
    assert_int_equal(default_sam_cfg_knb.mesh_hni_aux_ctl[4], 0x1000);
    assert_int_equal(default_sam_cfg_knb.mesh_hni_aux_ctl[5], 0x1000);

    assert_int_equal(default_sam_cfg_knb.mesh_hnt_dn_domain_cxra, 0x0);

    assert_int_equal(default_sam_cfg_knb.mesh_rni_cfg_ctl[0], 0xd000802001c21);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_cfg_ctl[1], 0xd000802001c21);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_cfg_ctl[2], 0xd000802001c21);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_cfg_ctl[3], 0xd000802001c21);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_cfg_ctl[4], 0xd004200401c41);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_cfg_ctl[5], 0xd000802001c71);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_cfg_ctl[6], 0xd000802001c71);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_cfg_ctl[7], 0xd000802001c71);

    assert_int_equal(default_sam_cfg_knb.mesh_rni_aux_ctl[0], 0x4004002);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_aux_ctl[1], 0x4004002);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_aux_ctl[2], 0x4004002);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_aux_ctl[3], 0x4004002);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_aux_ctl[4], 0x4004002);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_aux_ctl[5], 0x4004002);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_aux_ctl[6], 0x4004002);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_aux_ctl[7], 0x4004002);

    assert_int_equal(default_sam_cfg_knb.mesh_rnd_cfg_ctl[0], 0xd000802001c21);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_cfg_ctl[1], 0xd000802001c01);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_cfg_ctl[2], 0xd000802001c21);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_cfg_ctl[3], 0xd000802001c21);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_cfg_ctl[4], 0xd000802001c21);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_cfg_ctl[5], 0xd000200401c01);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_cfg_ctl[6], 0xd000802001c01);

    assert_int_equal(default_sam_cfg_knb.mesh_rnd_aux_ctl[0], 0x4004012);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_aux_ctl[1], 0x4004012);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_aux_ctl[2], 0x4004012);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_aux_ctl[3], 0x4004012);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_aux_ctl[4], 0x4004012);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_aux_ctl[5], 0x4004012);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_aux_ctl[6], 0x4004012);

    assert_int_equal(default_sam_cfg_knb.mesh_hnf_pocq_alloc_class_dedicated, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_pocq_alloc_class_max_allowed, 0x8203E3F);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_pocq_alloc_class_contended_min, 0x10101010);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_pocq_alloc_misc_max_allowed, 0x204);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_class_ctl, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_pocq_qos_class_ctl, 0x70B8ECFF);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_class_pocq_arb_weight_ctl, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_class_retry_weight_ctl, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_pocq_misc_retry_weight_ctl, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_sbsx_cbusy_limit_ctl, 0x483018);

    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[0].qos_control, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[0].qos_lat_tagt, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[0].qos_lat_scale, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[0].qos_lat_range, 0x0);

    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[1].qos_control, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[1].qos_lat_tagt, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[1].qos_lat_scale, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[1].qos_lat_range, 0x0);

    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[2].qos_control, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[2].qos_lat_tagt, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[2].qos_lat_scale, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[2].qos_lat_range, 0x0);

    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[0].qos_control, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[0].qos_lat_tagt, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[0].qos_lat_scale, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[0].qos_lat_range, 0x0);

    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[1].qos_control, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[1].qos_lat_tagt, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[1].qos_lat_scale, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[1].qos_lat_range, 0x0);

    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[2].qos_control, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[2].qos_lat_tagt, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[2].qos_lat_scale, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[2].qos_lat_range, 0x0);

    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[0].qos_control, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[0].qos_lat_tagt, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[0].qos_lat_scale, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[0].qos_lat_range, 0x0);

    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[1].qos_control, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[1].qos_lat_tagt, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[1].qos_lat_scale, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[1].qos_lat_range, 0x0);

    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[2].qos_control, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[2].qos_lat_tagt, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[2].qos_lat_scale, 0x0);
    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[2].qos_lat_range, 0x0);
}

void verify_mesh_ras_config_knobs(void)
{
    // Mesh RAS Config Knobs
    assert_int_equal(default_mesh_ras_cfg_knb.mesh_RAS_Error_Detection_Disable, false);
    assert_int_equal(default_mesh_ras_cfg_knb.mesh_RAS_Error_Deferment_Disable, false);
    assert_int_equal(default_mesh_ras_cfg_knb.mesh_RAS_Uncorrected_Error_Int_Disable, false);
    assert_int_equal(default_mesh_ras_cfg_knb.mesh_RAS_Fault_Handling_Int_Disable, false);
    assert_int_equal(default_mesh_ras_cfg_knb.mesh_RAS_Corrected_Error_Int_Disable, true);
    assert_int_equal(default_mesh_ras_cfg_knb.mesh_RAS_Parity_Error_Disable, false);
}

void verify_ccg_config_knobs(void)
{
    // Mesh CCG Config Knobs
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ha_aux_ctl, 0x3C0008);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ha_cfg_ctl, 0x40040);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ha_cxprtcl_link0_ctl, 0x1C0000);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ra_cfg_ctl, 0x7C1007);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ra_aux_ctl, 0x1B1F343CC3846);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ra_ccprtcl_link0_ctl, 0x2C00000);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ra_cbusy_limit_ctl, 0x181008);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccla_aux_ctl, 0x1220000000004);
}

void verify_d2d_config_knobs(void)
{
    // Mesh D2D Config Knobs
    assert_int_equal(unit_test_default_d2d_cfg.d2d_pll_divder, 2);
    assert_int_equal(unit_test_default_d2d_cfg.d2d_ref_divder, 2);
    assert_int_equal(unit_test_default_d2d_cfg.d2d_pll_fb_devider, 0x14);
    assert_int_equal(unit_test_default_d2d_cfg.d2d_ecc_cfg, 0);
    assert_int_equal(unit_test_default_d2d_cfg.d2d_tx_interface_clk_alignment, 0);
    assert_int_equal(unit_test_default_d2d_cfg.d2d_ras_enable, 0);
    assert_int_equal(unit_test_default_d2d_cfg.d2d_sleep_cfg, 1);
    assert_int_equal(unit_test_default_d2d_cfg.d2d_sleep_cfg_entry, 0);
    assert_int_equal(unit_test_default_d2d_cfg.d2d_rxcal_find_goodlanes_skip, 1);
}

// Test Mesh, CML, D2D Config Knobs
TEST_FUNCTION(test_mesh_config_knobs_single_die, setup_svp_platform, setup_undefined_platform)
{
    cmn800_sequence_params_t cmn800_sequence_param = {};

    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    g_test_mesh_d2d_override = true;

    // The function reading and updating is calling this
    expect_function_call(__wrap_cmn800_get_mesh_sam_cfg_knob);
    expect_function_call(__wrap_cmn800_get_mesh_ras_cfg_knob);
    expect_function_call(__wrap_get_ccg_knob_defaults);
    expect_function_call(__wrap_get_default_d2d_cfg);

    // Print Function is calling this
    expect_function_call(__wrap_cmn800_get_mesh_sam_cfg_knob);
    expect_function_call(__wrap_cmn800_get_mesh_ras_cfg_knob);
    expect_function_call(__wrap_get_ccg_knob_defaults);
    expect_function_call(__wrap_get_default_d2d_cfg);

    // Call API under test
    mesh_read_cfg_knobs_from_spi(&cmn800_sequence_param);

    // Check the kng_mscp_config_knobs.xml file for the values
    // Check if the defaults are set into mocked Silibs functions
    verify_mesh_config_knobs();

    verify_mesh_ras_config_knobs();

    verify_ccg_config_knobs();

    verify_d2d_config_knobs();
}

// Single Die Warm Reset Boot
TEST_FUNCTION(test_mesh_init_single_die_warm_reset_boot_Die_0_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_system_info_is_warm_start, true);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

// Dual Die Warm Reset Boot
TEST_FUNCTION(test_mesh_init_dual_die_warm_reset_boot_Die_0_SVP, setup_svp_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_system_info_is_warm_start, true);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

// Dual Die Warm Reset Boot
TEST_FUNCTION(test_mesh_init_dual_die_warm_reset_boot_Die_1_SVP, setup_svp_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    will_return(__wrap_system_info_is_warm_start, true);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

// Single Die Warm Reset Boot FPGA
TEST_FUNCTION(test_mesh_init_single_die_warm_reset_boot_Die_0_FPGA, setup_fpga_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_system_info_is_warm_start, true);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

// Dual Die Warm Reset Boot FPGA
TEST_FUNCTION(test_mesh_init_dual_die_warm_reset_boot_Die_0_FPGA, setup_fpga_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_system_info_is_warm_start, true);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

// Dual Die Warm Reset Boot FPGA
TEST_FUNCTION(test_mesh_init_dual_die_warm_reset_boot_Die_1_FPGA, setup_fpga_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    will_return(__wrap_system_info_is_warm_start, true);

    expect_function_call(__wrap_process_mesh_binary_from_spi);
    expect_function_call(__wrap_cmn800_generate_ddr_mc_map_from_snf_config);
    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

// Test Print Function
TEST_FUNCTION(test_mesh_init_print_numa_info, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    numa_cfg.NumaEnabled = true;
    // Set Non-Zero Values
    numa_cfg.PerDomainCfg[NumaDomainZero].DomainNum = 0x0;
    numa_cfg.PerDomainCfg[NumaDomainZero].MinIts = 0x3;
    numa_cfg.PerDomainCfg[NumaDomainZero].MaxIts = 0x5;
    numa_cfg.PerDomainCfg[NumaDomainOne].DomainNum = 0x1;
    numa_cfg.PerDomainCfg[NumaDomainOne].MinIts = 0x4;
    numa_cfg.PerDomainCfg[NumaDomainOne].MaxIts = 0x6;

    // Call API under test
    print_numa_info(&numa_cfg);
}
}
