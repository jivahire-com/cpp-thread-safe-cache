//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs.h
 * Implementation of AVS module and related functionality
 */

#pragma once

/*----------- Nested includes ------------*/

#include <avs_lib.h>
#include <stdint.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

#define AVS_ERROR_NONE          0x00
#define AVS_STATUS_MASK         0x01
#define AVS_ERR_ACK_MASK        0x03
#define AVS_VDONE               0x10  
#define AVS_NO_CONTROL          0x40  
#define AVS_ERROR_STATUS_ONLY (AVS_VDONE | AVS_NO_CONTROL)
#define AVS_ANY_ERROR         (~AVS_ERROR_STATUS_ONLY)

#define AVS_NO_CONTROL_SHIFT     (18)
#define AVS_ERR_STAT_ALERT_SHIFT (19)
#define AVS_VDONE_SHIFT          (20)
#define AVS_ERR_ACK_SHIFT        (21)

/*-------------- Typedefs ----------------*/

enum avs_internal_request_type_idx
{    
    AVS_REQUEST_READ_DATA,  // non _RESP events are triggered by the client
    AVS_REQUEST_WRITE_DATA,
    AVS_REQUEST_READ_ALL_VCT,
    AVS_REQUEST_READ_MULTI,
    AVS_REQUEST_WRITE_MULTI,
    AVS_REQUEST_WRITE_DATA_RESP,  // the _RESP events are triggered when the avs_libs interrupt occurs, TBD if these are needed
    AVS_REQUEST_READ_DATA_RESP,
    AVS_REQUEST_READ_ALL_VCT_RESP,
    AVS_REQUEST_READ_MULTI_RESP,
    AVS_REQUEST_WRITE_MULTI_RESP,
    AVS_REQUEST_COUNT
};

enum avs_bus_id
{
    AVS_BUS0,   // D0
    AVS_BUS1,   // D0
    AVS_BUS2,   // D0
    AVS_BUS3,   // D0
    AVS_BUS4,   // D1
    AVS_BUS_MAX
};

enum avs_sync_request_type
{
    AVS_GET_ERROR_COUNTS
};

typedef struct _avs_device_t {
  uint8_t dev_id;
} avs_device_t;

// bits 21 and 22 = Target ACK
// bits 16 - 20 = status response. (VDone bit 20, StatusAlert bit 19, AVS_Control bit 18, MfrSpec 17 and 16)
typedef struct _avs_error_t
{
    union {
        struct {
            uint8_t crc_error : 1;          // Interrupt CRC error
            uint8_t no_action_busy : 1;     // TargetAck = 0x01 Good CRC, no action taken, resource busy
            uint8_t bad_crc_no_action : 1;  // TargetAck = 0x10 Bad CRC, no action taken
            uint8_t invalid_no_action : 1;  // TargetAck = 0x11 Good CRC, invalid selector, data type or incorrect data. No action taken
            uint8_t v_done : 1;             // VDone - bit
            uint8_t status_alert : 1;       // StatusAlert bit 19 in response
            uint8_t no_control : 1;         // AVS StatusResponse bit (1 when controlling AVS output, 0 when not) set this bit when no control
       };
       uint8_t as_uint8;
    };
} avs_error_t, *pavs_error;

typedef struct _scp_avs_command_params_t {
    union {
        void *data_ptr;
        uint32_t avs_data;
    } data;
    uint8_t error;         
    uint8_t rail_id;             // specific rail or rail number to start reading (in avs_master_command_mem_start_t this is 'command_type')
    uint8_t rail_count_to_read;  // number of rails requested to be read
    uint8_t cmd_type : 4;        // commands (AVS_VOLTAGE_RW, AVS_CURRENT_READ, etc.), are 4 bits - extra bits can indicate special cases like v+c+t
    uint8_t rsvd : 3;            // unused
    uint8_t unused : 1;
} scp_avs_command_params_t;

typedef struct _scp_avs_config_t {
    /*! Interrupt number of the AVSBus */
    const unsigned int avs_irq;
    /*! Address of avs */
    uintptr_t reg_base_addr;
    /*! Number of rails on the specified avs*/
    uint8_t rail_count;
    /*! AFM CLOCK, drive strength range = 0 - 7 */
    uintptr_t afm_csr_avs_clk_addr;
    /*! MData, drive strength range = 0 - 7 */
    uintptr_t afm_csr_mdata_addr;
} scp_avs_config_t;

struct avs_element {
    /*! Element name */
    const char *name;
    /*!
     * \brief Pointer to element-specific configuration data for each AVS (scp_avs_config)
     */
    const void *data;   
} ;

typedef struct _scp_avs_vr_vct_t {
    uint16_t voltage_mV;      // 1LSB=1mV
    uint16_t current_cA;      // 1LSB=10mA
    uint16_t temperature_dC;  // 1LSB=0.1 Celsius
    uint8_t error_voltage;
    uint8_t error_current;
    uint8_t error_temperature;
} scp_avs_vr_vct_t;

typedef struct _scp_avs_error_count_t {
    uint16_t crc_error_count;
    uint16_t ack_no_action_busy_error_count;
    uint16_t ack_bad_crc_no_action_error_count;
    uint16_t ack_invalid_no_action_error_count;
    uint16_t status_alert_error_count;
} scp_avs_error_count_t, *pscp_avs_error_count;

/*--------- Function Prototypes ----------*/

/*!
 * \brief Checks the data read for errors.
 *
 * \details bits 21 and 22 = Target ACK
 * bits 16 - 20 = status response. (VDone bit 20, StatusAlert bit 19, AVS_Control bit 18, MfrSpec 17 and 16)
 * 
 *
 * \param resp_data Data read from the AVSBus following a read/write.
 * \retval error (i.e. AVS_ERROR_ACK_NO_ACTION_BUSY, AVS_ERROR_STATUS_ALERT, etc.).
 */
uint8_t scp_avs_status_error(uint32_t resp_data);