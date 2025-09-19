//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_dfwk.h
 * Header containing definitions for the dfwk portion of power service
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>
#include <accelip_id.h>
#include <assert.h>
#include <power_runconfig.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define POWER_IF_ALARM_ID_MAX 14

/*-------------- Typedefs ----------------*/

/**
 *  @brief Synchonous and Asynchronous Interface Request IDs
 */
typedef enum {
    CLI_COMMANDS_START_ID = 0x1000,
    CLI_COMMANDS_POWER_CONFIG,
    CLI_COMMANDS_POWER_SET,
    CLI_COMMANDS_POWER_STATUS,
    CLI_COMMANDS_POWER_LOG,
    CLI_COMMANDS_POWER_ACCEL,
} e_cli_power_command_id_t;

typedef enum {
    LOOP_DISABLE_MODE_SINGLE_DIE = 0,
    LOOP_DISABLE_MODE_DUAL_DIE
} loop_disable_mode_t;

// struct for the power service/device
typedef struct
{
    DFWK_DEVICE_HEADER header;
    DFWK_QUEUE default_queue;
} power_service_t, *ppower_service_t;

// struct for an interface to the power service
typedef struct
{
    DFWK_INTERFACE_HEADER header;
    ppower_service_t p_device;
} power_service_interface_t, *ppower_service_interface_t;

/* Definition of the power set subcommand arguments */

struct _desiredparams
{
   bool all;            // False - all , true - Core 
   uint8_t core;      // 0-74      
   uint8_t state;    // 0-31
   uint8_t throttle; 	    // 0- 7	
   uintptr_t cluster_pex_base_addr;
   uint8_t core_count;
};

struct _plimitparams
{
   bool all;        // False - all , true - Core 
   uint8_t core;      // 0-74       
   uint8_t state;     // 0-31
   uintptr_t cluster_pex_base_addr;  
   uint8_t core_count; 
};

struct _nominalparams
{
    uint8_t current_val;                    // sets the nominal pstate used in loop (does not affect DVFS/ACPI)   
    uint8_t previous_val;                    // previous value of nominal pstate used in loop    
};

struct _forcedparams
{
    uint8_t pstate;                          // sets forced pstate (0 - 31), after setting PMIN
    uint16_t ldodacin;                       // sets the ldodacin value for the forced pstate
};
struct _pstatefreq
{
    uint32_t frac_div;                       // sets the dco_frac_value for the forced pstate
    uint32_t freq_ctrl;                      // sets the freq_ctrl_value for the forced pstate
    uint16_t fb_div;                         // sets the dco_div_value for the forced pstate
    uint8_t pstate;                          // sets the forced pstate (0 - 31)
};

struct _loopdisparams
{
    uint16_t loopdis_bits;                   // sets loop disable bits (1-ctrl, 2-vrtelem, 4-pvttelem)
    loop_disable_mode_t mode;                            // sets the mode for loop disable (single/dual)
};

struct _alarmparams
{
    uintptr_t base_addr;
    uint16_t alarm_threshold;        // sets the temperature threshold for power management
    uint16_t hist_threshold;
    uint8_t core;                    // 0-64
    uint8_t alarm_id;                // 0 - 13
    uint8_t pex_group;                   // 0 or 1, group selector
    char ab_selector;                // Takes either 'a' or 'b'
    bool all;                        // False - all , true - Core 
    bool dual_die;                   // True - dual die, false - single die
};

struct _currthrottleparams
{
    bool all;                        // False - all , true - Core 
    uint8_t core;                   
    uint8_t curr_threshold_1;
    uint8_t curr_threshold_2;
    uint8_t curr_threshold_3;
};

struct _accelparams
{
    ACCEL_ID accel_id;
    uint8_t bw_reduction_perc;
    bool dual_bus;
};

typedef union 
{
    uint8_t      pmin_type;                      // set the pmin type (0 - 3)
    bool         io_thermal;                     // set the iothermal reg to trigger SOC_HOT, MEM_HOT, THERM_TRIP
	uint16_t     cap_val;                        // set the rack power cap (W)
	struct      _desiredparams  desiredparams;   // sets OS desired pstate register
	struct      _plimitparams  	plimitparams;         // sets plimit
    struct      _loopdisparams loopdis_params; // sets loop disable bits (1-ctrl, 2-vrtelem, 4-pvttelem)
    uint16_t    minupdate_val;                  // sets the minimum plimit update per loop iteration, 0 disables    
    struct      _nominalparams  nominalparams;  // sets the nominal pstate used in loop (does not affect DVFS/ACPI)
    uint16_t    racklimit;                      // sets the rack limit gpio for simulated implementations  
    struct     _forcedparams    forcedparams;   // sets forced pstate and ldodacin
    struct     _pstatefreq      pstatefreq;     // sets forced frequency for testing/verification via (dco_frac, freq_ctrl, dco_div)
    struct     _alarmparams     alarm_cfg;     // sets alarm & hist threshold for VR & Temp throttle
    struct     _currthrottleparams currthresh_params; // sets current thresholds for throttling
    struct     _accelparams     accelparams;    // accelerator parameters
} _pwrset_subcommand_args;

typedef struct _pwr_icc_cap_complete_payload_t {
    int result;
    uint16_t current_cap;
    uint16_t previous_cap;
} pwr_icc_cap_complete_payload_t;

typedef union 
{
    pwr_icc_cap_complete_payload_t pwr_icc_cap_result; // set the rack power cap (W)
	struct      _desiredparams  desiredparams;   // sets OS desired pstate register
	struct      _plimitparams  	plimitparams;         // sets plimit
    struct      _loopdisparams loopdis_params; // sets loop disable bits (1-ctrl, 2-vrtelem, 4-pvttelem)
    uint16_t    minupdate_val;                  // sets the minimum plimit update per loop iteration, 0 disables    
    struct      _nominalparams  nominalparams;  // sets the nominal pstate used in loop (does not affect DVFS/ACPI)
    uint16_t    racklimit;                      // sets the rack limit gpio for simulated implementations  
    struct     _forcedparams    forcedparams;   // sets forced pstate and ldodacin  
    struct     _pstatefreq      pstatefreq;     // sets forced frequency for a particular pstate (used for testing/verification)
    struct     _alarmparams     alarm_cfg;     // sets alarm & hist threshold for power management
    struct     _currthrottleparams currthresh_params; // sets current thresholds for throttling
    struct     _accelparams     accelparams;    // accelerator parameters
} _pwrset_response_val;

/* Structure for the async dfwk CLI request to the power interface */
typedef struct {
    DFWK_ASYNC_REQUEST_HEADER header;
    uint8_t die;

    char* sub_command;
    power_if_cmd_t power_ext_if_cmd_id;

    /* Structure elements for fetch data requests to power service*/
    union {
        void* p_requested_data;  

        _pwrset_response_val    pwrset_response_val;

        pwr_intparams_t          pwr_intparams;         

    } fetch_data;

    /* Structure elements for set data requests to power service*/
    _pwrset_subcommand_args pwrset_sub_command_args;

} power_service_cli_request_t, *ppower_service_cli_request_t;

typedef union _remote_pwrcli_request_union_t {
    struct {
        uint32_t icc_cmd_code;
        power_if_cmd_t power_ext_if_cmd_id; // command ID for the request
        _pwrset_subcommand_args pwrset_sub_command_args; // sub-command arguments for the request
    };
    uint32_t as_uint32[16];
} remote_pwrcli_request_t, *p_remote_pwrcli_request_t;

static_assert(sizeof(remote_pwrcli_request_t) == 64, "remote_pwrcli_request_t size must be 64 bytes");

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif