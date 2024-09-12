//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tower_mocks.c
 * Mock functions for tower sequence
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // IWYU pragma: keep
#include <atu_lib.h>
#include <cmocka.h> // IWYU pragma: keep
#include <fpfw_icc_base.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <hsp_firmware_headers.h> // for HSP_MAILBOX_CMD_BOOT_STATUS_NOTIFY, HspFirmwareIdScp...
#include <silibs_status.h>
#include <stddef.h>
#include <tower_sequence.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }
    // Keep mscp base non-zero to allow checking base address in UTs
    atu_map_entry->mscp_start_address = 0xffffffff;

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }

    return mock_type(int);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

int __wrap_tower_sequence_configure_towers(tower_sequence_soc_init_params_t* tower_sequence_param)
{
    check_expected(tower_sequence_param->tower_configure_fabric_apu);
    check_expected(tower_sequence_param->tower_configure_fabric_fmu);
    check_expected(tower_sequence_param->tower_fabric_tower_resolved_addr);

    check_expected(tower_sequence_param->tower_configure_periph_apu);
    check_expected(tower_sequence_param->tower_configure_periph_fmu);
    check_expected(tower_sequence_param->tower_periph_tower_resolved_addr);

    check_expected(tower_sequence_param->tower_configure_d2dss_apu);
    check_expected(tower_sequence_param->tower_configure_d2dss_fmu);
    check_expected(tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr);
    check_expected(tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr);

    check_expected(tower_sequence_param->tower_configure_vab_sam);
    check_expected(tower_sequence_param->tower_configure_vab_apu);
    check_expected(tower_sequence_param->tower_configure_vab_fmu);
    check_expected(tower_sequence_param->tower_vab_instances_enabled);

    check_expected(tower_sequence_param->tower_configure_rpss_sam);
    check_expected(tower_sequence_param->tower_configure_rpss_apu);
    check_expected(tower_sequence_param->tower_configure_rpss_fmu);
    check_expected(tower_sequence_param->tower_rpss_instances_enabled);

    check_expected(tower_sequence_param->tower_configure_sdmss_sam);
    check_expected(tower_sequence_param->tower_configure_sdmss_apu);
    check_expected(tower_sequence_param->tower_sdmss_isolation_enabled);
    check_expected(tower_sequence_param->tower_sdmss_tower_resolved_addr);

    check_expected(tower_sequence_param->tower_configure_ioss_sam);
    check_expected(tower_sequence_param->tower_configure_ioss_apu);
    check_expected(tower_sequence_param->tower_ioss_tower_resolved_addr);

    check_expected(tower_sequence_param->die_id);
    function_called();
    return 0;
}

fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size, size_t* output_recv_bytes)
{
    check_expected_ptr(icc_ctx);
    check_expected_ptr(payload_buffer);
    check_expected(buffer_size);
    check_expected_ptr(output_recv_bytes);

    kng_hsp_mailbox_msg *recv_msg = (kng_hsp_mailbox_msg *)payload_buffer;
    recv_msg->header.cmd = HSP_MAILBOX_CMD_POST_SCP_INIT_TOWER_CONFIG_RSP;
    recv_msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
    *output_recv_bytes = sizeof(kng_hsp_mailbox_msg);

    return mock_type(fpfw_status_t);
}

bool __wrap_system_info_is_hsp_present()
{
    return mock_type(bool);
}