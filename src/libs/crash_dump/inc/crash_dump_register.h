//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_register.h
 * APIs for registering data to put in crash dump
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Captures built-in Cortex M7 registers
 * 
 */
void crash_dump_register_core_registers();

/**
 * @brief Captures default crash registers
 * 
 */
void crash_dump_register_default_registers();

/**
 * @brief Captures core stack
 * 
 */
void crash_dump_register_core_stack();

/**
 * @brief Captures standard information
 * 
 */
void crash_dump_register_standard_info();