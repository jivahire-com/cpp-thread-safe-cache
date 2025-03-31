//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mts_platform_implementation_i.h
 * Private header for the Message Transfer Service
 * platform specific definitions
 */

 #pragma once

 /*----------- Nested includes ------------*/

#include <fpfw_status.h>
#include <icc_mhu.h>
#include <icc_platform_defines.h>
#include <idsw_kng.h>
#include <kng_icc_shared.h>

 /*-- Symbolic Constant Macros (defines) --*/

 /*-------------- Typedefs ----------------*/

 // ICC Base api's require the transport header
typedef struct
{
    icc_mhu_header_t header;
    union
    {
        dcp_msg_t dcp_msg;
        trp_msg_t trp_msg;
    };
} icc_arm_mhu_trnsprt_t, *p_icc_arm_mhu_trnsprt_t;

typedef struct
{
    large_fifo_mailbox_msg_header header;
    union
    {
        dcp_msg_t dcp_msg;
        trp_msg_t trp_msg;
    };
} icc_large_mbox_trnsprt_t, *p_icc_large_mbox_trnsprt_t;

typedef union
{
    icc_arm_mhu_trnsprt_t arm_mhu;
    icc_large_mbox_trnsprt_t large_mbox;
} icc_trp_msg_t, *p_icc_trp_msg_t;

 /*-- Declarations (Statics and globals) --*/

 /*--------- Function Prototypes ----------*/
