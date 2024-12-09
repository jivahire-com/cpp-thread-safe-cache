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
#include <power_runconfig.h>
#include <stdint.h>

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
} e_cli_power_command_id_t;

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

typedef union 
{
	uint16_t     cap_val;                        // set the rack power cap (W)
	struct      _desiredparams  desiredparams;   // sets OS desired pstate register
	struct      _plimitparams  	plimitparams;         // sets plimit
    uint16_t    loopdis_bits;                   // sets loop disable bits (1-ctrl, 2-vrtelem, 4-pvttelem)
    uint16_t    minupdate_val;                  // sets the minimum plimit update per loop iteration, 0 disables    
    struct      _nominalparams  nominalparams;  // sets the nominal pstate used in loop (does not affect DVFS/ACPI)
    uint16_t    racklimit;                      // sets the rack limit gpio for simulated implementations  
    
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
    uint16_t    loopdis_bits;                   // sets loop disable bits (1-ctrl, 2-vrtelem, 4-pvttelem)
    uint16_t    minupdate_val;                  // sets the minimum plimit update per loop iteration, 0 disables    
    struct      _nominalparams  nominalparams;  // sets the nominal pstate used in loop (does not affect DVFS/ACPI)
    uint16_t    racklimit;                      // sets the rack limit gpio for simulated implementations  
    
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



/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif