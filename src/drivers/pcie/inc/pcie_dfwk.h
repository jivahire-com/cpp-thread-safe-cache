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
#include <pcie_config_variable.h>
#include <silibs_platform.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PCIE_RPSS_COUNT     (4)
#define ROOT_PORTS_PER_RPSS (4)

/*-------------- Typedefs ----------------*/
/* Async operations supported by each root port on a pcie subsystem*/
typedef enum _pcie_rp_async_request_t
{
    INITIATE_LINK_TRAINING = 0x00,
    WAIT_FOR_EVENT,
    PCIE_MAX_ASYNC_REQ
} pcie_rp_async_request_t;

/* Sync operations supported by each root port on a pcie subsystem*/
typedef enum _pcie_rp_sync_request_t
{
    INITIAL_CONFIG_REQUEST = 0x00,
    PRE_RP_INIT_REQUEST,
    POST_RP_INIT_REQUEST,
    PCIE_MAX_SYNC_REQ
} pcie_rp_sync_request_t;

typedef struct _pcie_async_request_t
{
    DFWK_ASYNC_REQUEST_HEADER header; /* Ensure this is always the first field in this struct */
    RPSS_INSTANCE rpss_index;
    uint8_t rp_index;
    pcie_rp_async_request_t rp_op;
    silibs_status_t status;
    TX_TIMER expiry_timer;
} pcie_async_request_t;

typedef struct _pcie_sync_request_t
{
    DFWK_SYNC_REQUEST_HEADER header; /* Ensure this is always the first field in this struct */
    RPSS_INSTANCE rpss_index;
    uint8_t rp_index;
    pcie_rp_sync_request_t req_type;
    silibs_status_t status;
} pcie_sync_request_t;

typedef struct _pciess_device_t
{
    DFWK_DEVICE_HEADER header; /* Ensure this is always the first field in this struct */
    DFWK_QUEUE default_queue;
    DFWK_QUEUE per_rp_queue[ROOT_PORTS_PER_RPSS];
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
