//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pwr_tlm_scp_mock.c
 * Mock functions for pwr_tlm_scp  service
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <data_proc_tlm_cmpnt.h>
#include <fpfw_cfg_mgr.h>
#include <in_band_tlm_cmpnt_i.h>
#include <kng_soc_constants.h>
#include <message_transfer_service.h>
#include <power_runconfig.h>
#include <pwr_tlm_core_exchange.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
uint8_t _ProviderMetadata_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
uint8_t _ProviderMetadata_et_msdata_end;   // Pointer to the end   of the .ProviderMetadata section
uint8_t _EventMetadata_et_msdata_start;    // Pointer to the start of the .EventMetadata section
uint8_t _EventMetadata_et_msdata_end;      // Pointer to the end   of the .EventMetadata memory section

bool g_die_id_mocked = false;

/*------------- Functions ----------------*/

bool __wrap_mts_is_primary_instance(void)
{
    return mock_type(bool);
}

void __wrap_mts_client_forward_trp_msg(p_trp_msg_t trp_msg, trp_broadcast_t broadcast_option)
{
    FPFW_UNUSED(trp_msg);
    FPFW_UNUSED(broadcast_option);
    function_called();
}

void __wrap_mts_client_send_trp_response(p_trp_msg_t trp_msg)
{
    FPFW_UNUSED(trp_msg);
    function_called();
}

void __wrap_mts_client_send_new_trp_msg(p_trp_msg_t trp_msg)
{
    FPFW_UNUSED(trp_msg);
    function_called();
}

void __wrap_mts_client_send_dcp_notification(mts_client_id_t client_id, dcp_notification_type_t notification)
{
    FPFW_UNUSED(client_id);
    FPFW_UNUSED(notification);
    function_called();
}

uint8_t __wrap_mts_get_this_die_id(void)
{
    return mock_type(uint8_t);
}

mts_platform_core_id_t __wrap_mts_get_this_core_id(void)
{
    return 2;
}

void __wrap_mts_client_flush_incoming_queue(mts_client_id_t id)
{
    FPFW_UNUSED(id);
    function_called();
}

void __wrap_FpFwAssertWithArgs(int expression, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
    FPFW_UNUSED(expression);
    FPFW_UNUSED(arg0);
    FPFW_UNUSED(arg1);
    FPFW_UNUSED(arg2);
    FPFW_UNUSED(arg3);

    function_called();
}

UINT __wrap__txe_queue_flush(TX_QUEUE* queue_ptr)
{
    check_expected_ptr(queue_ptr);

    return mock_type(UINT);
}

UINT __wrap__txe_queue_create(TX_QUEUE* queue_ptr, CHAR* name_ptr, UINT message_size, VOID* queue_start, ULONG queue_size, UINT queue_control_block_size)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(name_ptr);
    check_expected(message_size);
    check_expected_ptr(queue_start);
    check_expected(queue_size);
    check_expected(queue_control_block_size);
    return mock_type(UINT);
}

UINT __wrap__txe_queue_send(TX_QUEUE* queue_ptr, VOID* source_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(source_ptr);
    check_expected(wait_option);
    return mock_type(UINT);
}

UINT __wrap__txe_queue_receive(TX_QUEUE* queue_ptr, VOID* destination_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(destination_ptr);
    check_expected(wait_option);

    size_t mockSize = mock_type(size_t);
    void* pMockData = mock_ptr_type(void*);

    if (destination_ptr && pMockData)
    {
        memcpy(destination_ptr, pMockData, mockSize); // NOLINT memcpy ok for mock
    }

    return mock_type(UINT);
}

UINT __wrap__txe_block_pool_create(TX_BLOCK_POOL* pool_ptr, CHAR* name_ptr, ULONG block_size, VOID* pool_start, ULONG pool_size, UINT pool_control_block_size)
{
    FPFW_UNUSED(pool_ptr);
    FPFW_UNUSED(name_ptr);
    FPFW_UNUSED(block_size);
    FPFW_UNUSED(pool_start);
    FPFW_UNUSED(pool_size);
    FPFW_UNUSED(pool_control_block_size);

    function_called();

    return mock_type(UINT);
}

UINT __wrap__txe_block_release(VOID* block_ptr)
{
    FPFW_UNUSED(block_ptr);
    function_called();

    return mock_type(UINT);
}

fpfw_status_t __wrap_mts_client_register(mts_client_id_t id, p_mts_client_t client)
{
    FPFW_UNUSED(id);
    FPFW_UNUSED(client);

    return FPFW_STATUS_SUCCESS;
}

int __wrap_ddrss_load_crypto_key(uint32_t mc, uint32_t msg, uint32_t timeout_us)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(msg);
    FPFW_UNUSED(timeout_us);

    return 0;
}

int __wrap_ddrss_set_power_telemetry_config(uint32_t mc, void* pwr_tel_cfg)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(pwr_tel_cfg);
    return mock_type(int);
}

int __wrap_ddrss_set_power_telemetry_filter(uint32_t mc, uint16_t event_idx, void* pwr_tel_evt_filter)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(event_idx);
    FPFW_UNUSED(pwr_tel_evt_filter);
    return mock_type(int);
}

int __wrap_ddrss_pmu_init(uint32_t mc, uint16_t pmu_idx)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(pmu_idx);
    return mock_type(int);
}

int __wrap_ddrss_pmu_set_event(uint32_t mc, uint16_t pmu_idx, void* evt_cfg)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(pmu_idx);
    FPFW_UNUSED(evt_cfg);
    return mock_type(int);
}

int __wrap_ddrss_pmu_enable(uint32_t mc, uint16_t pmu_idx, uint32_t enable)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(pmu_idx);
    FPFW_UNUSED(enable);
    return mock_type(int);
}

power_tlm_mpam_scp_knobs_t __wrap_config_get_pwr_tlm_mpam_scp_knobs(void)
{
    // Return a zero-initialized struct - the actual struct definition isn't needed for the test
    power_tlm_mpam_scp_knobs_t knobs;
    memset(&knobs, 0, sizeof(knobs));
    return knobs;
}

void __wrap_power_get_adclk_telem(power_adclk_tel_t* adclk_tel)
{
    FPFW_UNUSED(adclk_tel);
    // Zero out the structure for testing
    if (adclk_tel != NULL)
    {
        memset(adclk_tel, 0, sizeof(power_adclk_tel_t));
    }
}

void __wrap_pwr_tlm_core_exch_scp_write_droop_counts(uint64_t (*droop_count_array)[NUM_AP_CORES_PER_DIE])
{
    FPFW_UNUSED(droop_count_array);
    function_called();
}

void __wrap_pwr_tlm_core_exch_scp_write_mpam_pmu_counts(uint64_t* mpam_pmu_count_array)
{
    check_expected_ptr(mpam_pmu_count_array);
    function_called();
}

int __wrap_ddrss_pmu_read_counter_snapshot(uint32_t mc, uint16_t pmu_idx, uint64_t* pmn_cnt)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(pmu_idx);

    if (pmn_cnt != NULL)
    {
        *pmn_cnt = mock_type(uint64_t);
    }

    return mock_type(int);
}