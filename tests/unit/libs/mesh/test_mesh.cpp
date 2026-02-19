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
bool g_simulate_a1_stepping = false;
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
    .d2d_sleep_cfg = 0, // XML default value
    .d2d_sleep_cfg_entry = 100000,
    .d2d_rxcal_find_goodlanes_skip = 1,
};

ccg_cfg_t unit_test_default_ccg_cfg = {.por_ccg_ha_aux_ctl = 0x3C0008ULL,
                                       .por_ccg_ha_cfg_ctl = 0x40040ULL,
                                       .por_ccg_ha_cxprtcl_link0_ctl = 0x1C0000ULL,
                                       .por_ccg_ra_cfg_ctl = 0x7C4407ULL,
                                       .por_ccg_ra_aux_ctl = 0x1B1F343CC3846ULL,
                                       .por_ccg_ra_ccprtcl_link0_ctl = 0x2C00000ULL,
                                       .por_ccg_ra_cbusy_limit_ctl = 0x181008ULL,
                                       .por_ccla_aux_ctl = 0x1220000000004ULL};

/*------------- Functions ----------------*/
//! Mocks for mailbox primitives called inside hsp_send_recv_enable_smmu()
KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return g_test_die;
}

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
static int setup_soc_platform_dual_die(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_RVP_EVT_SILICON);
    simulate_single_die = false;
    return 0;
}

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

void __wrap_hm_submit_cper(uint16_t error_domain_idx,
                           acpi_error_severity_t err_severity,
                           acpi_cper_section_t* err_record_section,
                           uint32_t err_record_section_size)
{
    assert_int_equal(error_domain_idx, ACPI_ERROR_DOMAIN_MESH);
    check_expected(err_severity);
    assert_non_null(err_record_section);
    check_expected(err_record_section_size);
    function_called();
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

void __wrap_cmn800_error_injection(uint8_t node_type, uint8_t node_id, uint16_t node_control_reg, uint32_t err_inj, uint8_t byte_par_err_inj, uint8_t die_num)
{
    check_expected(node_type);
    check_expected(node_id);
    check_expected(node_control_reg);
    check_expected(err_inj);
    check_expected(byte_par_err_inj);
    check_expected(die_num);
    function_called();
}

void __wrap_cmn800_pseudo_fault_error_injection(uint8_t node_type,
                                                uint8_t node_id,
                                                uint16_t node_control_reg,
                                                uint32_t err_inj,
                                                uint32_t err_cnt_down,
                                                uint8_t die_num,
                                                bool non_secure)
{
    check_expected(node_type);
    check_expected(node_id);
    check_expected(node_control_reg);
    check_expected(err_inj);
    check_expected(err_cnt_down);
    check_expected(die_num);
    check_expected(non_secure);
    function_called();
}

void __wrap_interrupt_handler_mesh_ras_error(acpi_err_sec_generic_t* mesh_cper, bool fault, bool non_secure, uint8_t die_num)
{
    assert_non_null(mesh_cper);
    mesh_cper->err_status = mock_type(uint32_t);
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

void __wrap_ras_arm_agent_notify_warm(ras_agent_entity_t* agent)
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

silibs_status_t __wrap_ras_arm_agent_set_record_counter_thresholds_by_index(ras_agent_entity_t* agent,
                                                                            unsigned index,
                                                                            uint16_t cec,
                                                                            uint16_t ceco)
{
    check_expected(agent);
    check_expected(index);
    check_expected(cec);
    check_expected(ceco);
    function_called();
    return mock_type(int);
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
    record->status = BIT29; // Set to ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL
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

int __wrap_get_hns_sds_vector_from_hns_sparring(kng_hns_fuses_t* hns_fuses_sds)
{
    FPFW_UNUSED(hns_fuses_sds);
    function_called();
    return SILIBS_SUCCESS;
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

// Mock function for A1 stepping detection
bool __wrap_idhw_is_stepping_a1(void)
{
    return g_simulate_a1_stepping;
}

// Mock A1-specific config_get functions - using XML default values
uint64_t __wrap_config_get_a1_mesh_hnf_cbusy_resp_ctl(void)
{
    return 0x180400800040000ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_cbusy_sn_ctl(void)
{
    return 0x100001000200040ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_cbusy_write_limit_ctl(void)
{
    return 0x302010ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_cbusy_mode_ctl(void)
{
    return 0x3000ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_aux_ctl(void)
{
    return 0x2000001000200002ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_aux_ctl_1(void)
{
    return 0x10001005A900ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_cfg_ctl(void)
{
    return 0x2000C01738921000ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_lbt_cfg_ctl(void)
{
    return 0x7F7F09ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_lbt_aux_ctl(void)
{
    return 0x440000000000006ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_lbt_cbusy_ctl(void)
{
    return 0x0ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_pocq_alloc_class_dedicated(void)
{
    return 0x0ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_pocq_alloc_class_max_allowed(void)
{
    return 0x8203E3FULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_pocq_alloc_class_contended_min(void)
{
    return 0x10101010ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_pocq_alloc_misc_max_allowed(void)
{
    return 0x204ULL; // XML default value
}

uint64_t __wrap_config_get_a1_mesh_hnf_pocq_qos_class_ctl(void)
{
    return 0x70B8ECFFULL; // XML default value
}

// A1-specific CCG knobs
uint64_t __wrap_config_get_a1_por_ccg_ha_aux_ctl(void)
{
    return 0x3C0008ULL; // XML default value
}

uint64_t __wrap_config_get_a1_por_ccg_ha_cfg_ctl(void)
{
    return 0x40040ULL; // XML default value
}

uint64_t __wrap_config_get_a1_por_ccg_ra_cfg_ctl(void)
{
    return 0x7C4407ULL; // XML default value
}

uint64_t __wrap_config_get_a1_por_ccg_ra_aux_ctl(void)
{
    return 0x1B1F343CC3846ULL; // XML default value
}

uint64_t __wrap_config_get_a1_por_ccg_ra_ccprtcl_link0_ctl(void)
{
    return 0x2C00000ULL; // XML default value
}

uint64_t __wrap_config_get_a1_por_ccg_ra_cbusy_limit_ctl(void)
{
    return 0x181008ULL; // XML default value
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

// Mock A0-specific config_get functions (default behavior)
uint64_t __wrap_config_get_mesh_hnf_cbusy_limit_ctl(void)
{
    return 0x302010ULL;
}
uint64_t __wrap_config_get_mesh_hnf_cbusy_resp_ctl(void)
{
    return 0x180400800040000ULL;
}
uint64_t __wrap_config_get_mesh_hnf_cbusy_sn_ctl(void)
{
    return 0x100001000200040ULL;
}
uint64_t __wrap_config_get_mesh_hnf_cbusy_write_limit_ctl(void)
{
    return 0x302010ULL;
}
uint64_t __wrap_config_get_mesh_hnf_cbusy_mode_ctl(void)
{
    return 0x3000ULL;
}
uint64_t __wrap_config_get_mesh_hnf_aux_ctl(void)
{
    return 0x2000001000200002ULL;
}
uint64_t __wrap_config_get_mesh_hnf_aux_ctl_1(void)
{
    return 0x10001005A900ULL;
}
uint64_t __wrap_config_get_mesh_hnf_cfg_ctl(void)
{
    return 0x2000C01738921000ULL;
}
uint64_t __wrap_config_get_mesh_hnf_lbt_cfg_ctl(void)
{
    return 0x7F7F09ULL;
}
uint64_t __wrap_config_get_mesh_hnf_lbt_aux_ctl(void)
{
    return 0x440000000000006ULL;
}
uint64_t __wrap_config_get_mesh_hnf_lbt_cbusy_ctl(void)
{
    return 0x0ULL;
}

// POCQ allocation knobs A0
uint64_t __wrap_config_get_mesh_hnf_pocq_alloc_class_dedicated(void)
{
    return 0x0ULL; // XML default value
}
uint64_t __wrap_config_get_mesh_hnf_pocq_alloc_class_max_allowed(void)
{
    return 0x8203E3FULL; // XML default value
}
uint64_t __wrap_config_get_mesh_hnf_pocq_alloc_class_contended_min(void)
{
    return 0x10101010ULL; // XML default value
}
uint64_t __wrap_config_get_mesh_hnf_pocq_alloc_misc_max_allowed(void)
{
    return 0x204ULL; // XML default value
}
uint64_t __wrap_config_get_mesh_hnf_class_ctl(void)
{
    return 0x0ULL; // XML default value
}
uint64_t __wrap_config_get_mesh_hnf_pocq_qos_class_ctl(void)
{
    return 0x70B8ECFFULL; // XML default value
}
uint64_t __wrap_config_get_mesh_hnf_class_pocq_arb_weight_ctl(void)
{
    return 0x0ULL; // XML default value
}
uint64_t __wrap_config_get_mesh_hnf_class_retry_weight_ctl(void)
{
    return 0x0ULL; // XML default value
}
uint64_t __wrap_config_get_mesh_hnf_pocq_misc_retry_weight_ctl(void)
{
    return 0x0ULL; // XML default value
}
uint64_t __wrap_config_get_mesh_sbsx_cbusy_limit_ctl(void)
{
    return 0x483018ULL; // XML default value
}

// HNI, HNT, RNI, RND knobs (returning default values for simplicity)
mesh_hni_cfg_ctl_t __wrap_config_get_mesh_hni_cfg_ctl_knob(void)
{
    mesh_hni_cfg_ctl_t knob = {{0x1, 0x1, 0x1, 0x1, 0x1, 0x1}};
    return knob;
}
mesh_hni_aux_ctl_t __wrap_config_get_mesh_hni_aux_ctl_knob(void)
{
    mesh_hni_aux_ctl_t knob = {{0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000}};
    return knob;
}
uint64_t __wrap_config_get_mesh_hnt_dn_domain_cxra(void)
{
    return 0x0ULL;
}

mesh_rni_cfg_ctl_t __wrap_config_get_mesh_rni_cfg_ctl_knob(void)
{
    mesh_rni_cfg_ctl_t knob = {
        {0xd000802001c21ULL, 0xd000802001c21ULL, 0xd000802001c21ULL, 0xd000802001c21ULL, 0xd004200401c41ULL, 0xd000802001c71ULL, 0xd000802001c71ULL, 0xd000802001c71ULL}};
    return knob;
}
mesh_rni_aux_ctl_t __wrap_config_get_mesh_rni_aux_ctl_knob(void)
{
    mesh_rni_aux_ctl_t knob = {
        {0x4004002ULL, 0x4004002ULL, 0x4004002ULL, 0x4004002ULL, 0x4004002ULL, 0x4004002ULL, 0x4004002ULL, 0x4004002ULL}};
    return knob;
}

mesh_rnd_cfg_ctl_t __wrap_config_get_mesh_rnd_cfg_ctl_knob(void)
{
    mesh_rnd_cfg_ctl_t knob = {
        {0xd000802001c21ULL, 0xd000802001c01ULL, 0xd000802001c21ULL, 0xd000802001c21ULL, 0xd000802001c21ULL, 0xd000200401c01ULL, 0xd000802001c01ULL}};
    return knob;
}
mesh_rnd_aux_ctl_t __wrap_config_get_mesh_rnd_aux_ctl_knob(void)
{
    mesh_rnd_aux_ctl_t knob = {
        {0x4004012ULL, 0x4004012ULL, 0x4004012ULL, 0x4004012ULL, 0x4004012ULL, 0x4004012ULL, 0x4004012ULL}};
    return knob;
}

mesh_rni_qos_cfg_t __wrap_config_get_mesh_rni_qos_cfg_knob(void)
{
    mesh_rni_qos_cfg_t knob = {
        {{0x1ULL, 0x2ULL, 0x0ULL, 0x0ULL}, {0x0ULL, 0x0ULL, 0x0ULL, 0x0ULL}, {0x0ULL, 0x0ULL, 0x0ULL, 0x0ULL}}};
    return knob;
}
mesh_rnd_qos_cfg_t __wrap_config_get_mesh_rnd_qos_cfg_knob(void)
{
    mesh_rnd_qos_cfg_t knob = {
        {{0x1ULL, 0x2ULL, 0x0ULL, 0x0ULL}, {0x0ULL, 0x0ULL, 0x0ULL, 0x0ULL}, {0x0ULL, 0x0ULL, 0x0ULL, 0x0ULL}}};
    return knob;
}
mesh_mxp_qos_cfg_t __wrap_config_get_mesh_mxp_qos_cfg_knob(void)
{
    mesh_mxp_qos_cfg_t knob = {
        {{0x1ULL, 0x2ULL, 0x0ULL, 0x0ULL}, {0x0ULL, 0x0ULL, 0x0ULL, 0x0ULL}, {0x0ULL, 0x0ULL, 0x0ULL, 0x0ULL}}};
    return knob;
}

// RAS knobs A0
bool __wrap_config_get_mesh_RAS_Error_Detection_Disable(void)
{
    return false;
}
bool __wrap_config_get_mesh_RAS_Error_Deferment_Disable(void)
{
    return false;
}
bool __wrap_config_get_mesh_RAS_Uncorrected_Error_Int_Disable(void)
{
    return false;
}
bool __wrap_config_get_mesh_RAS_Fault_Handling_Int_Disable(void)
{
    return false;
}
bool __wrap_config_get_mesh_RAS_Corrected_Error_Int_Disable(void)
{
    return false; // XML default value
}
bool __wrap_config_get_mesh_RAS_Parity_Error_Disable(void)
{
    return false;
}

// CCG knobs A0
uint64_t __wrap_config_get_por_ccg_ha_aux_ctl(void)
{
    return 0x3C0008ULL;
}
uint64_t __wrap_config_get_por_ccg_ha_cfg_ctl(void)
{
    return 0x40040ULL;
}
uint64_t __wrap_config_get_por_ccg_ha_cxprtcl_link0_ctl(void)
{
    return 0x1C0000ULL;
}
uint64_t __wrap_config_get_por_ccg_ra_cfg_ctl(void)
{
    return 0x7C4407ULL;
}
uint64_t __wrap_config_get_por_ccg_ra_aux_ctl(void)
{
    return 0x1B1F343CC3846ULL;
}
uint64_t __wrap_config_get_por_ccg_ra_ccprtcl_link0_ctl(void)
{
    return 0x2C00000ULL;
}
uint64_t __wrap_config_get_por_ccg_ra_cbusy_limit_ctl(void)
{
    return 0x181008ULL;
}
uint64_t __wrap_config_get_por_ccla_aux_ctl(void)
{
    return 0x1220000000004ULL;
}

// D2D knobs A0
uint8_t __wrap_config_get_d2d_pll_divider(void)
{
    return 0x2;
}
uint8_t __wrap_config_get_d2d_ref_divider(void)
{
    return 0x2;
}
uint8_t __wrap_config_get_d2d_pll_fb_divider(void)
{
    return 0x14;
}
uint8_t __wrap_config_get_d2d_ecc_cfg(void)
{
    return 0;
}
uint8_t __wrap_config_get_d2d_tx_interface_clk_alignment(void)
{
    return 0;
}
uint8_t __wrap_config_get_d2d_sleep_cfg(void)
{
    return 0; // XML default value
}
uint32_t __wrap_config_get_d2d_sleep_cfg_entry(void)
{
    return 100000;
}
uint8_t __wrap_config_get_d2d_rxcal_find_goodlanes_skip(void)
{
    return 1;
}
uint8_t __wrap_config_get_d2d_close_fb_wa(void)
{
    return 0;
}

// Additional config_get functions needed for mesh_read_cfg_knobs_from_spi
d2dss_sys_counter_delay_t __wrap_config_get_d2dss_system_counter_delay(void)
{
    d2dss_sys_counter_delay_t delay = {{0, 0, 0, 0, 0, 0, 0, 0}};
    return delay;
}

uint8_t __wrap_config_get_cmn_sam_config(void)
{
    return CONFIG_2D_NUMA_64HNS_HIER_3SN_enum;
}

cxl_region_params_t __wrap_config_get_cxl_params_die0(void)
{
    cxl_region_params_t params = {.interleave_ways = (CXL_INTERLEAVE_WAYS)2, .interleave_size = (CXL_INTERLEAVE_SIZE)3};
    return params;
}

cxl_region_params_t __wrap_config_get_cxl_params_die1(void)
{
    cxl_region_params_t params = {.interleave_ways = (CXL_INTERLEAVE_WAYS)2, .interleave_size = (CXL_INTERLEAVE_SIZE)3};
    return params;
}

bool __wrap_config_get_cmn_uma_arsm_htg_wa_disabled(void)
{
    return false;
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

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);

    will_return(__wrap_system_info_is_warm_start, false);

    // Setup the common ISR Expectations
    setup_common_isr_expectations();

    setup_common_numa_variables_expectations(test_die);
    // Call API under test
    d2d_init(test_die, test_icc_base_hsp_mbx_ctx);
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

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);

    will_return(__wrap_system_info_is_warm_start, false);

    // Setup the common ISR Expectations
    setup_common_isr_expectations();

    setup_common_numa_variables_expectations(test_die);
    // Call API under test
    d2d_init(test_die, test_icc_base_hsp_mbx_ctx);
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

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);

    will_return(__wrap_system_info_is_warm_start, false);

    // Setup the common ISR Expectations
    setup_common_isr_expectations();

    setup_common_numa_variables_expectations(test_die);
    // Call API under test
    d2d_init(test_die, test_icc_base_hsp_mbx_ctx);
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

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);

    will_return(__wrap_system_info_is_warm_start, false);

    // Setup the common ISR Expectations
    setup_common_isr_expectations();

    setup_common_numa_variables_expectations(test_die);
    // Call API under test
    d2d_init(test_die, test_icc_base_hsp_mbx_ctx);
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

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);

    will_return(__wrap_system_info_is_warm_start, false);

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
        will_return(__wrap_system_info_is_warm_start, false);
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
    d2d_init(test_die, test_icc_base_hsp_mbx_ctx);
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

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);

    will_return(__wrap_system_info_is_warm_start, false);

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
        will_return(__wrap_system_info_is_warm_start, false);
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
    d2d_init(test_die, test_icc_base_hsp_mbx_ctx);
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

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);

    will_return(__wrap_system_info_is_warm_start, false);

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
        will_return(__wrap_system_info_is_warm_start, false);
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
    d2d_init(test_die, test_icc_base_hsp_mbx_ctx);
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

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);

    will_return(__wrap_system_info_is_warm_start, false);

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
        will_return(__wrap_system_info_is_warm_start, false);
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
    d2d_init(test_die, test_icc_base_hsp_mbx_ctx);
}

// Mesh Error ISR Die 0
TEST_FUNCTION(test_mesh_error_handler_mesh_error_isr_Die_0_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    will_return(__wrap_interrupt_handler_mesh_ras_error, 0);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_INFORMATIONAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    // Call API under test
    mesh_error_isr(NULL);
}

TEST_FUNCTION(test_mesh_error_handler_mesh_error_isr_Die_0_SVP_UE, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    will_return(__wrap_interrupt_handler_mesh_ras_error, 0x20000000UL); // UE bit set
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    expect_value(__wrap_crash_dump_bug_check, errorCode, (uint32_t)KNG_E_RAS_MESH_ERROR_UE);
    expect_any(__wrap_crash_dump_bug_check, p1);
    expect_any(__wrap_crash_dump_bug_check, p2);
    expect_any(__wrap_crash_dump_bug_check, p3);
    expect_any(__wrap_crash_dump_bug_check, p4);
    expect_function_call(__wrap_crash_dump_bug_check);

    // Call API under test
    mesh_error_isr(NULL);
}

// Mesh Error ISR Die 1
TEST_FUNCTION(test_mesh_error_handler_mesh_error_isr_Die_1_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    will_return(__wrap_interrupt_handler_mesh_ras_error, 0);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_INFORMATIONAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    // Call API under test
    mesh_error_isr(NULL);
}

// Mesh Fault ISR Die 0
TEST_FUNCTION(test_mesh_error_handler_mesh_fault_isr_Die_0_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    will_return(__wrap_interrupt_handler_mesh_ras_error, 0);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_INFORMATIONAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    // Call API under test
    mesh_fault_isr(NULL);
}

TEST_FUNCTION(test_mesh_error_handler_mesh_fault_isr_Die_0_SVP_UE, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    will_return(__wrap_interrupt_handler_mesh_ras_error, 0x20000000UL); // UE bit set
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    expect_value(__wrap_crash_dump_bug_check, errorCode, (uint32_t)KNG_E_RAS_MESH_FAULT_UE);
    expect_any(__wrap_crash_dump_bug_check, p1);
    expect_any(__wrap_crash_dump_bug_check, p2);
    expect_any(__wrap_crash_dump_bug_check, p3);
    expect_any(__wrap_crash_dump_bug_check, p4);
    expect_function_call(__wrap_crash_dump_bug_check);

    // Call API under test
    mesh_fault_isr(NULL);
}

// Mesh Fault ISR Die 1
TEST_FUNCTION(test_mesh_error_handler_mesh_fault_isr_Die_1_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    will_return(__wrap_interrupt_handler_mesh_ras_error, 0);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_INFORMATIONAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    // Call API under test
    mesh_fault_isr(NULL);
}

// Mesh NS Error ISR Die 0
TEST_FUNCTION(test_mesh_error_handler_mesh_ns_error_isr_Die_0_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    will_return(__wrap_interrupt_handler_mesh_ras_error, 0);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_INFORMATIONAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    // Call API under test
    mesh_ns_error_isr(NULL);
}

TEST_FUNCTION(test_mesh_error_handler_mesh_ns_error_isr_Die_0_SVP_UE, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    will_return(__wrap_interrupt_handler_mesh_ras_error, 0x20000000UL); // UE bit set
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    expect_value(__wrap_crash_dump_bug_check, errorCode, (uint32_t)KNG_E_RAS_MESH_NON_SECURE_ERROR_UE);
    expect_any(__wrap_crash_dump_bug_check, p1);
    expect_any(__wrap_crash_dump_bug_check, p2);
    expect_any(__wrap_crash_dump_bug_check, p3);
    expect_any(__wrap_crash_dump_bug_check, p4);
    expect_function_call(__wrap_crash_dump_bug_check);

    // Call API under test
    mesh_ns_error_isr(NULL);
}

// Mesh NS Error ISR Die 1
TEST_FUNCTION(test_mesh_error_handler_mesh_ns_error_isr_Die_1_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    will_return(__wrap_interrupt_handler_mesh_ras_error, 0);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, false);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_INFORMATIONAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    // Call API under test
    mesh_ns_error_isr(NULL);
}

// Mesh NS Fault ISR Die 0
TEST_FUNCTION(test_mesh_error_handler_mesh_ns_fault_isr_Die_0_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    will_return(__wrap_interrupt_handler_mesh_ras_error, 0);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_INFORMATIONAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    // Call API under test
    mesh_ns_fault_isr(NULL);
}

TEST_FUNCTION(test_mesh_error_handler_mesh_ns_fault_isr_Die_0_SVP_UE, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    will_return(__wrap_interrupt_handler_mesh_ras_error, 0x20000000UL); // UE bit set
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    expect_value(__wrap_crash_dump_bug_check, errorCode, (uint32_t)KNG_E_RAS_MESH_NON_SECURE_FAULT_UE);
    expect_any(__wrap_crash_dump_bug_check, p1);
    expect_any(__wrap_crash_dump_bug_check, p2);
    expect_any(__wrap_crash_dump_bug_check, p3);
    expect_any(__wrap_crash_dump_bug_check, p4);
    expect_function_call(__wrap_crash_dump_bug_check);

    // Call API under test
    mesh_ns_fault_isr(NULL);
}

// Mesh NS Fault ISR Die 1
TEST_FUNCTION(test_mesh_error_handler_mesh_ns_fault_isr_Die_1_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;
    will_return(__wrap_interrupt_handler_mesh_ras_error, 0);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, fault, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, non_secure, true);
    expect_value(__wrap_interrupt_handler_mesh_ras_error, die_num, test_die);
    expect_function_call(__wrap_interrupt_handler_mesh_ras_error);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_INFORMATIONAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

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
        expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
        expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
        expect_function_call(__wrap_hm_submit_cper);
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

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

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
    will_return(__wrap_ras_arm_agent_probe, 0);                // error code
    will_return(__wrap_ras_arm_agent_probe, true);             // error code valid
    will_return(__wrap_ras_arm_agent_probe, true);             // return
    expect_function_call(__wrap_ras_arm_agent_probe);
    expect_function_call(__wrap_ras_print_record);

    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    expect_value(__wrap_crash_dump_bug_check, errorCode, (uint32_t)KNG_E_RAS_MESH_D2D_UE);
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
        will_return(__wrap_system_info_is_warm_start, false);
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

    // Override the g_test_d2d_ecc_ce_counter to test the D2D RAS init
    g_test_d2d_ecc_ce_counter = 0x30;

    // d2d_ras_init
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_setup_entity, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_setup_entity);
        will_return(__wrap_system_info_is_warm_start, false);
        expect_value(__wrap_ras_arm_agent_init, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_init);
        expect_value(__wrap_ras_arm_agent_enable_signaling_by_type, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_signaling_by_type);
        expect_value(__wrap_ras_arm_agent_enable_fhi, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_fhi);
        expect_value(__wrap_ras_arm_agent_enable_logging, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_logging);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, agent, &d2dss2_agent[d2d_subsystem]);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, index, 0);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index,
                     cec,
                     (g_test_d2d_ecc_ce_counter | RAS_ARM_COUNTER_SET_REQUEST));
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, ceco, 0);
        will_return(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, SILIBS_SUCCESS);
        expect_function_call(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, agent, &d2dss2_agent[d2d_subsystem]);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, index, 1);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index,
                     cec,
                     (g_test_d2d_ecc_ce_counter | RAS_ARM_COUNTER_SET_REQUEST));
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, ceco, 0);
        will_return(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, SILIBS_SUCCESS);
        expect_function_call(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index);
    }

    // Call API under test
    d2d_ras_init();
}

// D2D RAS Init for Die 0 Warm Reset Path
TEST_FUNCTION(test_mesh_error_handler_d2d_ras_init_warm_reset_Die_0, setup_svp_platform, setup_undefined_platform)
{
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    // Override the g_test_d2d_ecc_ce_counter to test the D2D RAS init
    g_test_d2d_ecc_ce_counter = 0x30;

    // d2d_ras_init
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_setup_entity, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_setup_entity);
        will_return(__wrap_system_info_is_warm_start, true);
        expect_value(__wrap_ras_arm_agent_notify_warm, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_notify_warm);
        expect_value(__wrap_ras_arm_agent_init, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_init);
        expect_value(__wrap_ras_arm_agent_enable_signaling_by_type, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_signaling_by_type);
        expect_value(__wrap_ras_arm_agent_enable_fhi, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_fhi);
        expect_value(__wrap_ras_arm_agent_enable_logging, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_enable_logging);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, agent, &d2dss2_agent[d2d_subsystem]);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, index, 0);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index,
                     cec,
                     (g_test_d2d_ecc_ce_counter | RAS_ARM_COUNTER_SET_REQUEST));
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, ceco, 0);
        will_return(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, SILIBS_SUCCESS);
        expect_function_call(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, agent, &d2dss2_agent[d2d_subsystem]);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, index, 1);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index,
                     cec,
                     (g_test_d2d_ecc_ce_counter | RAS_ARM_COUNTER_SET_REQUEST));
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, ceco, 0);
        will_return(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, SILIBS_SUCCESS);
        expect_function_call(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index);
    }

    // Call API under test
    d2d_ras_init();
}

// D2D RAS Set the CE Counter via CLI
TEST_FUNCTION(test_mesh_error_handler_d2d_ecc_ce_counter_update_Die_0, setup_svp_platform, setup_undefined_platform)
{
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;

    // Override the g_test_d2d_ecc_ce_counter to test the D2D RAS init
    g_test_d2d_ecc_ce_counter = 0x30;

    // d2d_ras_init
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_set_base, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_set_base);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, agent, &d2dss2_agent[d2d_subsystem]);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, index, 0);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index,
                     cec,
                     (g_test_d2d_ecc_ce_counter | RAS_ARM_COUNTER_SET_REQUEST));
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, ceco, 0);
        will_return(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, SILIBS_SUCCESS);
        expect_function_call(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, agent, &d2dss2_agent[d2d_subsystem]);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, index, 1);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index,
                     cec,
                     (g_test_d2d_ecc_ce_counter | RAS_ARM_COUNTER_SET_REQUEST));
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, ceco, 0);
        will_return(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, SILIBS_SUCCESS);
        expect_function_call(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index);
        // Call API under test
        d2d_ecc_ce_counter_update(d2d_subsystem, g_test_d2d_ecc_ce_counter);
    }
}

// D2D RAS Set the CE Counter via CLI
TEST_FUNCTION(test_mesh_error_handler_d2d_ecc_ce_counter_update_Die_1, setup_svp_platform, setup_undefined_platform)
{
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;

    // Override the g_test_d2d_ecc_ce_counter to test the D2D RAS init
    g_test_d2d_ecc_ce_counter = 0x31;

    // d2d_ras_init
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_set_base, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_set_base);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, agent, &d2dss2_agent[d2d_subsystem]);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, index, 0);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index,
                     cec,
                     (g_test_d2d_ecc_ce_counter | RAS_ARM_COUNTER_SET_REQUEST));
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, ceco, 0);
        will_return(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, SILIBS_SUCCESS);
        expect_function_call(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, agent, &d2dss2_agent[d2d_subsystem]);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, index, 1);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index,
                     cec,
                     (g_test_d2d_ecc_ce_counter | RAS_ARM_COUNTER_SET_REQUEST));
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, ceco, 0);
        will_return(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, SILIBS_SUCCESS);
        expect_function_call(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index);
        // Call API under test
        d2d_ecc_ce_counter_update(d2d_subsystem, g_test_d2d_ecc_ce_counter);
    }

    // Go out of bounds
    d2d_ecc_ce_counter_update(NUM_OF_CCG_WITH_D2D, g_test_d2d_ecc_ce_counter);
}

// D2D RAS Set the CE Counter via CLI Test Failure Paths
TEST_FUNCTION(test_mesh_error_handler_d2d_ecc_ce_counter_update_Die_1_Failed_Path, setup_svp_platform, setup_undefined_platform)
{
    const auto test_die = (KNG_DIE_ID)1;
    g_test_die = test_die;

    // Override the g_test_d2d_ecc_ce_counter to test the D2D RAS init
    g_test_d2d_ecc_ce_counter = 0x32;

    // d2d_ras_init
    for (uint8_t d2d_subsystem = 0; d2d_subsystem < NUM_OF_CCG_WITH_D2D; d2d_subsystem++)
    {
        expect_value(__wrap_ras_arm_agent_set_base, agent, &d2dss2_agent[d2d_subsystem]);
        expect_function_call(__wrap_ras_arm_agent_set_base);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, agent, &d2dss2_agent[d2d_subsystem]);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, index, 0);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index,
                     cec,
                     (g_test_d2d_ecc_ce_counter | RAS_ARM_COUNTER_SET_REQUEST));
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, ceco, 0);
        will_return(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, SILIBS_E_PARAM);
        expect_function_call(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, agent, &d2dss2_agent[d2d_subsystem]);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, index, 1);
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index,
                     cec,
                     (g_test_d2d_ecc_ce_counter | RAS_ARM_COUNTER_SET_REQUEST));
        expect_value(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, ceco, 0);
        will_return(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index, SILIBS_E_PARAM);
        expect_function_call(__wrap_ras_arm_agent_set_record_counter_thresholds_by_index);
        // Call API under test
        d2d_ecc_ce_counter_update(d2d_subsystem, g_test_d2d_ecc_ce_counter);
    }
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
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_aux_ctl_1, 0x10001005A900);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cfg_ctl, 0x2000C01738921000);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_lbt_cfg_ctl, 0x7F7F09);
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

    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[0].qos_control, 0x1);
    assert_int_equal(default_sam_cfg_knb.mesh_rni_qos_cfg[0].qos_lat_tagt, 0x2);
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

    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[0].qos_control, 0x1);
    assert_int_equal(default_sam_cfg_knb.mesh_rnd_qos_cfg[0].qos_lat_tagt, 0x2);
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

    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[0].qos_control, 0x1);
    assert_int_equal(default_sam_cfg_knb.mesh_mxp_qos_cfg[0].qos_lat_tagt, 0x2);
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
    assert_int_equal(default_mesh_ras_cfg_knb.mesh_RAS_Corrected_Error_Int_Disable, false);
    assert_int_equal(default_mesh_ras_cfg_knb.mesh_RAS_Parity_Error_Disable, false);
}

void verify_ccg_config_knobs(void)
{
    // Mesh CCG Config Knobs
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ha_aux_ctl, 0x3C0008ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ha_cfg_ctl, 0x40040ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ha_cxprtcl_link0_ctl, 0x1C0000ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ra_cfg_ctl, 0x7C4407ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ra_aux_ctl, 0x1B1F343CC3846ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ra_ccprtcl_link0_ctl, 0x2C00000ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ra_cbusy_limit_ctl, 0x181008ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccla_aux_ctl, 0x1220000000004ULL);
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
    assert_int_equal(unit_test_default_d2d_cfg.d2d_sleep_cfg, 0);
    assert_int_equal(unit_test_default_d2d_cfg.d2d_sleep_cfg_entry, 100000);
    assert_int_equal(unit_test_default_d2d_cfg.d2d_rxcal_find_goodlanes_skip, 1);
}

// A1-specific verification functions
void verify_mesh_config_knobs_a1_stepping(void)
{
    // Verify A1-specific mesh HNF knobs are applied (XML default values)
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cbusy_resp_ctl, 0x180400800040000ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cbusy_sn_ctl, 0x100001000200040ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cbusy_write_limit_ctl, 0x302010ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cbusy_mode_ctl, 0x3000ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_aux_ctl, 0x2000001000200002ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_aux_ctl_1, 0x10001005A900ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cfg_ctl, 0x2000C01738921000ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_lbt_cfg_ctl, 0x7F7F09ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_lbt_aux_ctl, 0x440000000000006ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_lbt_cbusy_ctl, 0x0ULL);

    // Verify A1-specific POCQ allocation knobs (XML default values)
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_pocq_alloc_class_dedicated, 0x0ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_pocq_alloc_class_max_allowed, 0x8203E3FULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_pocq_alloc_class_contended_min, 0x10101010ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_pocq_alloc_misc_max_allowed, 0x204ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_pocq_qos_class_ctl, 0x70B8ECFFULL);

    // Verify knobs that should remain unchanged (A0 values)
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_cbusy_limit_ctl, 0x302010ULL);
    assert_int_equal(default_sam_cfg_knb.mesh_hnf_class_ctl, 0x0ULL);
}

void verify_ccg_config_knobs_a1_stepping(void)
{
    // Verify A1-specific CCG knobs are applied (XML default values)
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ha_aux_ctl, 0x3C0008ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ha_cfg_ctl, 0x40040ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ra_cfg_ctl, 0x7C4407ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ra_aux_ctl, 0x1B1F343CC3846ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ra_ccprtcl_link0_ctl, 0x2C00000ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ra_cbusy_limit_ctl, 0x181008ULL);

    // Verify knobs that should remain unchanged (A0 values)
    assert_int_equal(unit_test_default_ccg_cfg.por_ccg_ha_cxprtcl_link0_ctl, 0x1C0000ULL);
    assert_int_equal(unit_test_default_ccg_cfg.por_ccla_aux_ctl, 0x1220000000004ULL);
}

// Test Mesh, CML, D2D Config Knobs
TEST_FUNCTION(test_mesh_config_knobs_single_die, setup_soc_platform_dual_die, setup_undefined_platform)
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
    // expect_function_call(__wrap_cmn800_get_mesh_sam_cfg_knob);
    // expect_function_call(__wrap_cmn800_get_mesh_ras_cfg_knob);
    // expect_function_call(__wrap_get_ccg_knob_defaults);
    // expect_function_call(__wrap_get_default_d2d_cfg);

    // Call API under test
    mesh_read_cfg_knobs_from_spi(&cmn800_sequence_param);

    // Check the kng_mscp_config_knobs.xml file for the values
    // Check if the defaults are set into mocked Silibs functions
    verify_mesh_config_knobs();

    verify_mesh_ras_config_knobs();

    verify_ccg_config_knobs();

    verify_d2d_config_knobs();
}

// Test Mesh Config Knobs with A1 Stepping - A0 stepping (default)
TEST_FUNCTION(test_mesh_config_knobs_a0_stepping, setup_soc_platform_dual_die, setup_undefined_platform)
{
    cmn800_sequence_params_t cmn800_sequence_param = {};

    // Set up expectations for A0 stepping
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    g_test_mesh_d2d_override = true;
    g_simulate_a1_stepping = false; // A0 stepping (default)

    // The function reading and updating is calling this
    expect_function_call(__wrap_cmn800_get_mesh_sam_cfg_knob);
    expect_function_call(__wrap_cmn800_get_mesh_ras_cfg_knob);
    expect_function_call(__wrap_get_ccg_knob_defaults);
    expect_function_call(__wrap_get_default_d2d_cfg);

    // Call API under test
    mesh_read_cfg_knobs_from_spi(&cmn800_sequence_param);

    // Verify A0 (default) knob values are used
    verify_mesh_config_knobs();
    verify_mesh_ras_config_knobs();
    verify_ccg_config_knobs();
    verify_d2d_config_knobs();
}

// Test Mesh Config Knobs with A1 Stepping - A1 stepping
TEST_FUNCTION(test_mesh_config_knobs_a1_stepping, setup_soc_platform_dual_die, setup_undefined_platform)
{
    cmn800_sequence_params_t cmn800_sequence_param = {};

    // Set up expectations for A1 stepping
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    g_test_mesh_d2d_override = true;
    g_simulate_a1_stepping = true; // A1 stepping

    // The function reading and updating is calling this
    expect_function_call(__wrap_cmn800_get_mesh_sam_cfg_knob);
    expect_function_call(__wrap_cmn800_get_mesh_ras_cfg_knob);
    expect_function_call(__wrap_get_ccg_knob_defaults);
    expect_function_call(__wrap_get_default_d2d_cfg);

    // Call API under test
    mesh_read_cfg_knobs_from_spi(&cmn800_sequence_param);

    // Verify A1-specific knob values are used
    verify_mesh_config_knobs_a1_stepping();
    verify_mesh_ras_config_knobs(); // RAS knobs remain the same
    verify_ccg_config_knobs_a1_stepping();
    verify_d2d_config_knobs(); // D2D knobs remain the same
}

// Test Mesh Config Knobs with A1 Stepping - Override disabled
TEST_FUNCTION(test_mesh_config_knobs_a1_stepping_override_disabled, setup_soc_platform_dual_die, setup_undefined_platform)
{
    cmn800_sequence_params_t cmn800_sequence_param = {};

    // Set up expectations for A1 stepping but override disabled
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    g_test_mesh_d2d_override = false; // Override disabled - should not apply any knobs
    g_simulate_a1_stepping = true;    // A1 stepping

    // Even when override is disabled, these functions are still called
    expect_function_call(__wrap_cmn800_get_mesh_sam_cfg_knob);
    expect_function_call(__wrap_cmn800_get_mesh_ras_cfg_knob);
    expect_function_call(__wrap_get_ccg_knob_defaults); // Called for debug printing
    expect_function_call(__wrap_get_default_d2d_cfg);

    // Call API under test
    mesh_read_cfg_knobs_from_spi(&cmn800_sequence_param);

    // When override is disabled, no knobs should be applied, so no verification needed
    // This test mainly verifies the code doesn't crash when override is disabled
    assert_int_equal(cmn800_sequence_param.die_num, test_die);
    assert_int_equal(cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_64HNS_HIER_3SN_enum);
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

// Test Mesh RAS Health Monitor Module
TEST_FUNCTION(test_mesh_error_injection_cb_err_inj_1D, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    ras_einj_info_t einj_payload = {0x0};

    // hm_inject_err <group> <type> <instance> <status_operation> <err_type> <err_severity> <err_addr>
    // <err_value> hm_inject_err 0x1 0x0 0x0 0x0 0x10C 0x0 0x40d 0x1
    einj_payload.component_group = ACPI_ERROR_DOMAIN_MESH;
    einj_payload.component_type = COMPONENT_TYPE_MESH;
    einj_payload.component_instance = test_die;
    einj_payload.status_operation.value = OPERATION_STATUS_ERR_INJ;
    einj_payload.param_type.error_parameters[0] = 0x10C;
    einj_payload.param_type.error_parameters[1] = 0x40d;
    einj_payload.value_type.data_64 = 0x1;

    expect_value(__wrap_cmn800_error_injection, node_type, 0x1);
    expect_value(__wrap_cmn800_error_injection, node_id, 0xC);
    expect_value(__wrap_cmn800_error_injection, node_control_reg, 0x40d);
    expect_value(__wrap_cmn800_error_injection, err_inj, 0x1);
    expect_value(__wrap_cmn800_error_injection, byte_par_err_inj, 0x0);
    expect_value(__wrap_cmn800_error_injection, die_num, test_die);

    expect_function_call(__wrap_cmn800_error_injection);
    // Call API under test
    mesh_error_injection_cb(&einj_payload, NULL);
}

// Test Mesh RAS Health Monitor Module
TEST_FUNCTION(test_mesh_error_injection_cb_pseudo_fault_err_inj_1D, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    ras_einj_info_t einj_payload = {0x0};

    // hm_inject_err <group> <type> <instance> <status_operation> <err_type> <err_severity> <err_addr>
    // <err_value>
    // hm_inject_err 0x1 0x0 0x0 0x1 0x10C 0x0 0x401 0x100080000A20
    einj_payload.component_group = ACPI_ERROR_DOMAIN_MESH;
    einj_payload.component_type = COMPONENT_TYPE_MESH;
    einj_payload.component_instance = test_die;
    einj_payload.status_operation.value = OPERATION_STATUS_PSEUDO_FAULT_ERR_GEN;
    einj_payload.param_type.error_parameters[0] = 0x10C;
    einj_payload.param_type.error_parameters[1] = 0x401;
    einj_payload.value_type.data_64 = 0x100080000A20;

    expect_value(__wrap_cmn800_pseudo_fault_error_injection, node_type, 0x1);
    expect_value(__wrap_cmn800_pseudo_fault_error_injection, node_id, 0xC);
    expect_value(__wrap_cmn800_pseudo_fault_error_injection, node_control_reg, 0x401);
    expect_value(__wrap_cmn800_pseudo_fault_error_injection, err_inj, 0x80000A20);
    expect_value(__wrap_cmn800_pseudo_fault_error_injection, err_cnt_down, 0x1000);
    expect_value(__wrap_cmn800_pseudo_fault_error_injection, die_num, test_die);
    expect_value(__wrap_cmn800_pseudo_fault_error_injection, non_secure, 0x0);

    expect_function_call(__wrap_cmn800_pseudo_fault_error_injection);
    // Call API under test
    mesh_error_injection_cb(&einj_payload, NULL);
}

// Test Mesh RAS Health Monitor Module
TEST_FUNCTION(test_mesh_error_injection_cb_d2d_ras_error_inj_1D, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    ras_einj_info_t einj_payload = {0x0};

    // hm_inject_err <group> <type> <instance> <status_operation> <err_type> <err_severity> <err_addr>
    // <err_value>
    // hm_inject_err 0x1 0x1 0x0 0x0 0x0 0x0 0x0 0xF000000102000
    einj_payload.component_group = ACPI_ERROR_DOMAIN_MESH;
    einj_payload.component_type = COMPONENT_TYPE_D2D;
    einj_payload.component_instance = test_die;
    einj_payload.status_operation.value = OPERATION_STATUS_ERR_INJ;
    einj_payload.param_type.error_parameters[0] = 0x1; // D2D Subsystem 1
    einj_payload.param_type.error_parameters[1] = 0x0;
    einj_payload.value_type.data_64 = 0xF000000102000;

    expect_value(__wrap_ras_arm_agent_set_base, agent, &d2dss2_agent[1]);
    expect_function_call(__wrap_ras_arm_agent_set_base);
    expect_value(__wrap_ras_arm_agent_trigger_by_type, agent, &d2dss2_agent[1]);
    expect_function_call(__wrap_ras_arm_agent_trigger_by_type);

    // Call API under test
    mesh_error_injection_cb(&einj_payload, NULL);
}

// Test for Failures in the CB function 1
TEST_FUNCTION(test_mesh_error_injection_cb_failure_1, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    ras_einj_info_t einj_payload = {0x0};
    acpi_einj_cmd_status_t status = ACPI_EINJ_SUCCESS;

    einj_payload.component_group = ACPI_ERROR_DOMAIN_MESH + 1; // Invalid component group for this CB fnc

    // Call API under test
    status = mesh_error_injection_cb(&einj_payload, NULL);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);
}

// Test for Failures in the CB function 2
TEST_FUNCTION(test_mesh_error_injection_cb_failure_2, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    acpi_einj_cmd_status_t status = ACPI_EINJ_SUCCESS;

    // Call API under test
    status = mesh_error_injection_cb(NULL, NULL);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);
}

// Test for Failures in the CB function 3
TEST_FUNCTION(test_mesh_error_injection_cb_failure_3, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    ras_einj_info_t einj_payload = {0x0};
    acpi_einj_cmd_status_t status = ACPI_EINJ_SUCCESS;

    einj_payload.component_group = ACPI_ERROR_DOMAIN_MESH;
    einj_payload.component_type = COMPONENT_TYPE_MAX; // Invalid component type for this CB fnc

    // Call API under test
    status = mesh_error_injection_cb(&einj_payload, NULL);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);
}

// Test for Failures in the CB function 4
TEST_FUNCTION(test_mesh_error_injection_cb_failure_4, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    ras_einj_info_t einj_payload = {0x0};
    acpi_einj_cmd_status_t status = ACPI_EINJ_SUCCESS;

    einj_payload.component_group = ACPI_ERROR_DOMAIN_MESH;
    einj_payload.component_type = COMPONENT_TYPE_MESH;
    einj_payload.component_instance = test_die;
    einj_payload.status_operation.value = OPERATION_STATUS_MAX; // Invalid status operation for this CB fnc

    // Call API under test
    status = mesh_error_injection_cb(&einj_payload, NULL);
    assert_int_equal(status, ACPI_EINJ_INVALID_ACCESS);
}

// Test for SDS API
TEST_FUNCTION(test_mesh_get_hns_sds_vector_from_hns_sparring, setup_soc_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    int status = 0x0;
    kng_hns_fuses_t DIE0_hns_fuses = {0x0};
    expect_function_call(__wrap_get_hns_sds_vector_from_hns_sparring);
    // Call API under test
    status = mesh_get_hns_sds_vector_from_hns_sparring(&DIE0_hns_fuses);
    assert_int_equal(status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_mesh_get_hns_sds_vector_from_hns_sparring_svp, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    g_test_die = test_die;
    int status = 0x0;
    kng_hns_fuses_t DIE0_hns_fuses = {0x0};
    // Call API under test
    status = mesh_get_hns_sds_vector_from_hns_sparring(&DIE0_hns_fuses);
    assert_int_equal(status, SILIBS_SUCCESS);
    assert_int_equal(DIE0_hns_fuses.hns_fuses_31_0, MESH_DEFAULT_HNS_FUSES_31_0);
    assert_int_equal(DIE0_hns_fuses.hns_fuses_63_32, MESH_DEFAULT_HNS_FUSES_63_32);
    assert_int_equal(DIE0_hns_fuses.hns_fuses_95_64, MESH_DEFAULT_HNS_FUSES_95_64);
}
}
