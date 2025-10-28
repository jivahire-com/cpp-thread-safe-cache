//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_remote_die_i.h
 * Header containing definitions for power remote die exchanges
 */

#pragma once

/*----------- Nested includes ------------*/
#include "pid_resource.h" // for pid_context_t
#include "power_i.h"
#include "power_loops_i.h"
#include "power_runconfig_i.h"

#include <fpfw_status.h>       // for FPFW_STATUS_SUCCESS, FPFW_STATUS_INVA...
#include <kng_error.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/**
 * @brief Struct consolidating the power data to be exchanged between dies
 * residing in ARSM region. This includes arsm meta data + actual power event data
 *
 */
typedef struct _power_d2d_arsm_data
{
    bool d2d_pwr_data_sync; //! Flag/status governing the shared mem sync, Set by sender, cleared by receiver
    uint64_t timestamp;     //!< timestamp for the data
    uint32_t transaction_count; //! unique transaction count for the data
    uint32_t crc; //!< CRC for the data
    size_t pwr_data_size;   //!< size of the data
    //! Cmd code & dest etc info not required to be part of the meta data
    //! As each die & cmd code combo has a separate allocated space in the ARSM region, see below
    uint32_t pwr_data_addr[];    //! ref to power "event" data in the ARSM region given below eg input exchange/complete etc
} power_d2d_arsm_data_t;

/**
 * @brief struct consolidating the power "event" data to be exchanged between dies
 * for `ICC_COMMAND_PWR_D2D_EX_INPUT_MSG`
 * @note Any additional data to be sent to the remote die should be added here
 */
typedef struct _power_d2d_data_exchange_input
{
    uint16_t vrcpu_cap_die0; //!< VCPU cap from die 0, die 1 needs to use this value
    power_capping_mode_t capping_mode; //!< Do we need this? Capping mode, we may already have this info from knobs on each die
    /**
     * @brief contains the data to be sent to the remote die
     * 1. Latest power calculations from local die
     * 2. Throttle/boost core count data used in resource distribution
     * 3. max resources or the total number of perf levels we'd need to raise every core to highest/max perf,
     * assuming all cores can go to highest P state
     */
    power_remote_data_t remote_data_snapshot; //!< Snapshot of remote core's data, current core sends local data & recvs remote core's data
} power_d2d_data_ex_input_t;

/**
 * @brief struct consolidating the power data to be exchanged between dies
 * for `ICC_COMMAND_PWR_D2D_EX_COMPLETE_MSG`
 * @note Any additional data to be sent to the remote die should be added here
 */
typedef struct _power_d2d_data_exchange_complete
{
    pid_context_t pid_context; //!< on mismatch die1 will update to die0 value w/ pid_set_context AND both sides will log to power trace
    power_loop_plimit_stats_t plimit_stats; //!< Plimit selection stats
    bool is_currently_throttling;
    //! What else do we need to send?
} power_d2d_data_ex_complete_t;

/*-- Additional Macros  --*/
//! Macros to define the size & address of d2d powr data for each die, arsm meta data + actual power event data size
#define D2D_PWR_MESG_INPUT_EXCHANGE_SIZE (sizeof(power_d2d_arsm_data_t) + sizeof(power_d2d_data_ex_input_t))
#define D2D_PWR_MESG_COMPLETE_EXCHANGE_SIZE \
    (sizeof(power_d2d_arsm_data_t) + sizeof(power_d2d_data_ex_complete_t))
#define D2D_PWR_MESG_MAX_EXCHANGE_SIZE \
    ((D2D_PWR_MESG_INPUT_EXCHANGE_SIZE > D2D_PWR_MESG_COMPLETE_EXCHANGE_SIZE) ? \
    D2D_PWR_MESG_INPUT_EXCHANGE_SIZE : D2D_PWR_MESG_COMPLETE_EXCHANGE_SIZE)
//! D0 sends D1 recvs, input exchange mesg
#define D2D_PWR_MESG_INPUT_EXCHANGE_BASE_D0 (D1_ARSM_MSCP_D2D_POWER_DATA_BASE)
#define D2D_PWR_MESG_INPUT_EXCHANGE_END_D0 \
    (D2D_PWR_MESG_INPUT_EXCHANGE_BASE_D0 + D2D_PWR_MESG_INPUT_EXCHANGE_SIZE - 1)
//! D0 sends D1 recvs, complete exchange mesg
#define D2D_PWR_MESG_COMPLETE_EXCHANGE_BASE_D0 (D2D_PWR_MESG_INPUT_EXCHANGE_END_D0 + 1)
#define D2D_PWR_MESG_COMPLETE_EXCHANGE_END_D0 \
    (D2D_PWR_MESG_COMPLETE_EXCHANGE_BASE_D0 + D2D_PWR_MESG_COMPLETE_EXCHANGE_SIZE - 1)
//! D1 sends D0 recvs, input exchange mesg
#define D2D_PWR_MESG_INPUT_EXCHANGE_BASE_D1 (D2D_PWR_MESG_COMPLETE_EXCHANGE_END_D0 + 1)
#define D2D_PWR_MESG_INPUT_EXCHANGE_END_D1 \
    (D2D_PWR_MESG_INPUT_EXCHANGE_BASE_D1 + D2D_PWR_MESG_INPUT_EXCHANGE_SIZE - 1)
//! D1 sends D0 recvs, complete exchange mesg
#define D2D_PWR_MESG_COMPLETE_EXCHANGE_BASE_D1 (D2D_PWR_MESG_INPUT_EXCHANGE_END_D1 + 1)
#define D2D_PWR_MESG_COMPLETE_EXCHANGE_END_D1 \
    (D2D_PWR_MESG_COMPLETE_EXCHANGE_BASE_D1 + D2D_PWR_MESG_COMPLETE_EXCHANGE_SIZE - 1)

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize remote die power exchange
 *
 * @param[in] p_runconfig - Pointer to runtime configuration structure
 *
 */
void power_remote_die_init(power_runconfig_t* p_runconfig);

/**
 * @brief Function to exchange collected data with remote die
 * 
 * @param[in] p_runconfig - Pointer to runtime configuration structure
 */
void power_remote_die_exchange_inputs(power_runconfig_t* p_runconfig);

/**
 * @brief Function to exchange completion data with remote die
 * 
 * @param[in] p_runconfig - Pointer to runtime configuration structure
 */
void power_remote_die_exchange_complete(power_runconfig_t* p_runconfig);

/**
 * @brief Function to reset the remote die exchange state
 * 
 */
void power_remote_die_idle_reset();

/**
 * @brief Function to reset the remote die exchange state in case of error
 * 
 * @param[in] last_state 
 */
void power_remote_die_error_reset(int last_state);

/**
 * @brief Get the arsm mem to write to. Use only for test purposes.
 * 
 * @param d2d_ctx 
 * @return power_d2d_arsm_data_t* 
 */
power_d2d_arsm_data_t* get_arsm_mem_to_write(void* d2d_ctx);

/**
 * @brief Get the arsm mem to read object. Use only for test purposes.
 * 
 * @param d2d_ctx 
 * @return power_d2d_arsm_data_t* 
 */
power_d2d_arsm_data_t* get_arsm_mem_to_read(void* d2d_ctx);

#ifdef __cplusplus
}
#endif
