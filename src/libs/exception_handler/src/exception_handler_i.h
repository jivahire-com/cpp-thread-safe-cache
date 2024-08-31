//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file exception_handler_i.h
 * Private header file of exception handler
 */
#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>     // for uint32_t
#include <tx_api.h>     // for TX_THREAD
#include <tx_thread.h>  // for TX_THREAD

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
// Registers pushed to the stack on exception entry
// https://developer.arm.com/documentation/ddi0403/d/System-Level-Architecture/System-Level-Programmers--Model/ARMv7-M-exception-model/Exception-entry-behavior?lang=en
typedef struct __attribute__((__packed__)) {
    uint32_t R0;
    uint32_t R1;
    uint32_t R2;
    uint32_t R3;
    uint32_t R12;
    uint32_t LR;
    uint32_t PC;
    uint32_t PSR;
} exception_stack_frame_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void exception_handler(exception_stack_frame_t* stack_frame);
void threadx_stack_error_handler(TX_THREAD* tx_thread);
