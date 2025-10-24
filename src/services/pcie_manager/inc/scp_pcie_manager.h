//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * SCP service responsible for PCIe root port subsystem management.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkThreadXHost.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h>
#include <pcie_config_variable.h>
#include <pcie_dfwk.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PCIE_OUTSTANDING_REQ_QUOTA        (4) /* This allows up to 4 outstanding async. requests per rpss */
#define MAX_PENDING_WAIT_FOR_EVENT_PER_RP (2) /* Max number of pending wait for event Req per RP */

/*-------------- Typedefs ----------------*/

/* PCIe completion request format - this is what is used by the service for internal communication */
typedef struct _pciess_completion_request_t
{
    pcie_rp_async_request_t op;
    uint8_t rp_index;
    silibs_status_t status;
    pcie_async_data_t async_data;
} pciess_completion_request_t;

/* This is the thread context block for each RPSS management thread */
typedef struct _pcie_manager_context_t
{
    RPSS_INSTANCE rpss_idx;
    pciess_device_t* dev;
    pciess_device_interface_t* iface;
    pcie_async_request_t async_req[PCIESS_NUM_PORTS * MAX_PENDING_WAIT_FOR_EVENT_PER_RP];
    pciess_completion_request_t cmpl_req[PCIE_OUTSTANDING_REQ_QUOTA];
    TX_THREAD worker;
    TX_QUEUE work_queue;
    TX_EVENT_FLAGS_GROUP* event_ptr;
    TX_EVENT_FLAGS_GROUP* phyfw_load_event_ptr;
    TX_TIMER expiry_timer[PCIESS_NUM_PORTS];
    uint8_t lt_retry_count[PCIESS_NUM_PORTS];
    bool is_cold_boot;
    uint8_t update_pds[PCIESS_NUM_PORTS];
} pcie_manager_context_t;

/* This is the thread context block for config service thread*/
typedef struct _pcie_config_manager_context_t
{
    TX_THREAD worker;
    uint16_t event_flags_mask;
    TX_EVENT_FLAGS_GROUP* event_ptr;
    kingsgate_pcie_root_bridge_config* rb_config_var;
    kingsgate_pcie_vab_config* vab_config_var;
} pcie_config_manager_context_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief      Used to initialize the PCIe subsystem management service and
 *              drivers on SCP.
 *
 *  @param[in]  schedule  Driver framework scheduler being used by the host.
 *  @param[in]  rpss_to_init Number of RPSS being initialized
 *
 *  @retval     Array of PCIe device handles for all four RPSS on the system
 *              Cab be used by dependent services - like the PCIe CLI to
 *              interface with the pcie driver.
 *
 *              Errors in rpss init are fatal and are raised directly through
 *              FpFwErrorRaise when encountered.
 */
void* scp_pcie_initialize(PDFWK_SCHEDULE schedule, uint16_t rpss_to_init, KNG_DIE_ID die_id);

/**
 *  @brief      Start the PCIe configuration service thread after dependencies have been initialized.
 *
 *
 *  @retval     None. Errors raised if failure in initialization.
 */
void scp_pcie_start_config_service_thread(void);

/**
 *  @brief      Returns true if all PCIess on this die are disabled.
 *
 *  @param[in]  None
 *
 *  @retval     true if all PCIess on this die are disabled, false otherwise.
 */
bool scp_pcie_is_disabled(void);
