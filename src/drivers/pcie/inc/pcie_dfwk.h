//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * PCIe root port driver client interface definitions.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkCommon.h>
#include <DfwkThreadXHost.h>
#include <FpFwAssert.h>
#include <kng_soc_constants.h>
#include <pcie_common.h>
#include <pcie_config_variable.h>
#include <silibs_platform.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/* Async operations supported by each root port on a pcie subsystem*/
typedef enum _pcie_rp_async_request_t
{
    WAIT_FOR_EVENT = 0x00,
    LINK_TRAINING_FAILED,
    PCIE_MAX_ASYNC_REQ
} pcie_rp_async_request_t;

/* Sync operations supported by each root port on a pcie subsystem*/
typedef enum _pcie_rp_sync_request_t
{
    INITIAL_CONFIG_REQUEST = 0x00,
    PRE_RP_INIT_REQUEST,
    POST_RP_INIT_REQUEST,
    INITIATE_LINK_TRAINING,
    POST_RP_LINK_UP_INIT,
    SET_SECONDARY_BUS_RESET_REQUEST,
    CLEAR_SECONDARY_BUS_RESET_REQUEST,
    GET_RPSS_ENTITY_REQUEST,
    GET_RPSS_READY_REQUEST,
    GET_RP_READY_REQUEST,
    GET_LINK_STATUS,
    FORCE_AER_INTERNAL_UNCORR_ERROR,
    IDE_TX_REKEY,
    IDE_RX_REKEY,
    INJECT_PCIE_ERROR,
    CLI_REQUEST,
    SET_PDS_SHADOW_REGISTER,
    PCIE_MAX_SYNC_REQ
} pcie_rp_sync_request_t;

/*
 * For now async callback just has a INT mask
 * and two data fields passed back to the service.
 */
typedef struct _pcie_async_data_t
{
    uint32_t int_mask;
    uint64_t glbl_ide_data;
    uint64_t aes_hcfg_data;
} pcie_async_data_t;

typedef enum _pcie_cli_req_op_t
{
    GET_RP_LTSSM_STATE = 0,
    GET_RP_LINK_STATE,
    GET_RP_DBI_CONFIG_HDR,
    MAX_PCIE_CLI_REQ_OP
} pcie_cli_req_op_t;

typedef struct _pcie_async_request_t
{
    DFWK_ASYNC_REQUEST_HEADER header; /* Ensure this is always the first field in this struct */
    RPSS_INSTANCE rpss_index;
    uint8_t rp_index;
    pcie_rp_async_request_t rp_op;
    silibs_status_t status;
    pcie_async_data_t async_data;
} pcie_async_request_t;

typedef struct _pcie_sync_request_t
{
    DFWK_SYNC_REQUEST_HEADER header; /* Ensure this is always the first field in this struct */
    RPSS_INSTANCE rpss_index;
    uint8_t rp_index;
    pcie_rp_sync_request_t req_type;
    silibs_status_t status;
    void* p_sent_data;
    void* p_requested_data;
} pcie_sync_request_t;

typedef struct _pciess_device_t
{
    DFWK_DEVICE_HEADER header; /* Ensure this is always the first field in this struct */
    DFWK_QUEUE default_queue;
    DFWK_QUEUE per_rp_queue[PCIESS_NUM_PORTS];
    pcie_root_bridge_config* rb_configs; // pointer to rb config
} pciess_device_t;

typedef struct _pciess_device_interface_t
{
    DFWK_INTERFACE_HEADER header;
    pciess_device_t* dev;
} pciess_device_interface_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief Initialize and register a PCIe root port subsystem device with
 *         the driver framework (dfwk).
 *
 *  @param[in] dev       Pointer to a PCIe device which is to be registered.
 *
 *  @param[in] schedule  Pointer to a DFWK_SCHEDULE type scheduler object
 *                       belonging to the host.
 *
 *  @return None         Any failures in PCIe init are catastrophic and will
 *                       report runtime asserts/crashes instead of returning.
 */
void pcie_dfwk_init(pciess_device_t* dev, PDFWK_SCHEDULE schedule);

/**
 *  @brief Open up and provide an interface to access a PCIe rpss device.
 *
 *  @param[in]      dev    Pointer to a PCIe device for which the interface is
 *                         to be opened.
 *
 *  @param[in, out] iface  Pointer to a PCIe interface object owned by the
 *                         calling entity. It will be initialized by dfwk and
 *                         returned.
 *
 *  @return None
 */
void pcie_dfwk_interface_init(pciess_device_t* dev, pciess_device_interface_t* iface);
