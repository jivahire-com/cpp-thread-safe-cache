//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
 
/**
* @file power_interrupt_handling.h
* Header file for PLL interrupt handling in the power service.
*/
#pragma once
#include <fpfw_icc_base.h>
 
/*----------- Nested includes ------------*/
 
/*-- Symbolic Constant Macros (defines) --*/
 
/*-------------- Typedefs ----------------*/
 
/*-- Declarations (Statics and globals) --*/
 
/*--------- Function Prototypes ----------*/
/**
* @brief Enable PLL error interrupts.
*/
void enable_pll_interrupts();
void core_pll_error_status(uint32_t core_idx, bool is_unlock);
void pll_isr(void);