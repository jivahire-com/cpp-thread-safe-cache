//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file fuse_main_i.h
 * Header containing internal definitions for fuse service
 */

#pragma once
/*----------- Nested includes ------------*/
#include <stdbool.h>
#include <stdio.h>
#include <fpfw_status.h> // for fpfw_status_t

/*------------- Typedefs -----------------*/
/*-------- Function Prototypes -----------*/

void scp_remote_die_config_req_cb(void* context, fpfw_status_t status);
void save_remote_die_config_cb(void* context, size_t output_size_bytes, fpfw_status_t status);
int prepare_remote_die_config_listener(fpfw_icc_base_ctx_t* icc_ctx);


/*-- Declarations (Statics and globals) --*/
/*------------- Functions ----------------*/
