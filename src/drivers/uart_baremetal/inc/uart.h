//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file Uart.h
 * Uart public API
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stddef.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
// https://microsoft.sharepoint.com/:x:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/SOC%20Top/Kingsgate%20SOC%20Address%20Map%20r1p04.xlsx?d=w1dbe548890594d609964d4ae084eee3a&csf=1&web=1&e=GPYjgq
#define UART0BASE_SCP     (uint32_t)0x44002000U
#define UART0BASE_MCP     (uint32_t)0x4C002000U
#define UART0BASE_SDM     (uint32_t)0x40B9F000U
#define UART0BASE_CDEDSDM (uint32_t)0x40B9F000U

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

void UartInit(uint32_t base_addr);
void UartWrite(void *pSrc, size_t Length);
uint32_t UartRead(void *pSrc, size_t Length);
