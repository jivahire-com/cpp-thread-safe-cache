/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file ap_core_test.h
 *
 */

#pragma once

/*----------- Includes ------------*/

#include <tx_api.h>

#define WRAPPER_ICC_COMMAND 		1
#define WRAPPER_ICC_COMMAND_FAIL    2

#define SIZE_OF_MAILBOX_TEST        256
#define SIZE_OF_MSG_BUFFER          64

#define ICC_MSG_INDEX_SEND          0
#define ICC_MSG_INDEX_RECEIVE       1

#define ICC_MSG_SCMI_RECEIVE_ID     0x0504 

extern UINT timer_active_status;

/*--------- Function Prototypes ----------*/


