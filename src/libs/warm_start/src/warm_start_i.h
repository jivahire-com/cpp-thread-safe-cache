//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mod_warm_start_i.h
 * Private warm start header
 */

#pragma once

#include <warm_start_id.h>

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/
#define MODULE_NAME "[WS] "
#define NEWLINE     "\n"
#define WARM_START_MAGIC_ID 0x12345678

/* TODO Update the magic number after https://dev.azure.com/ms-tsd/Kingsgate/_git/silibs/pullrequest/187912 is merged*/
#define SCP_WARM_START_BASE 0x01290000UL
#define SCP_WARM_START_SIZE 65536

#define WS_LOG_INFO(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define WS_LOG_ERR(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

/*-------------- Typedefs ----------------*/
typedef struct _ws_data_entry_t {
    struct _ws_data_entry_t *p_next;
    mod_ws_data_id_t id;  // name space assigned id
    int32_t checksum;     // 2s complement checksum
    uint32_t size;        // Length of the data present
    uint8_t data;         // Start of data for this id
} ws_data_entry_t;

typedef struct _ws_data_list_t_ {
    uint32_t magic_id;
    uint32_t version;
    ws_data_entry_t entry;
} ws_data_list_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
