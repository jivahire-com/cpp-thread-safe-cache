//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file Uart.c
 * This file contains the implementation for the UART
 */

/*------------- Includes -----------------*/
#include "uart.h"

#include "uart_i.h"

// Tell cspell to ignore register names
/* cSpell:ignore LCRL LCRM LCRH */

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static uint32_t uart_base_addr = UART0BASE_SCP;

/*-------------- Functions ---------------*/

/**
 * Initialization API for UART
 *
 * @retval
 *      None
 */
void UartInit(uint32_t base_addr)
{
    uart_base_addr = base_addr;

    // Disable the serial port while setting the baud rate and word length
    UART0_CR = 0;

    // Set the correct baud rate and word length. TODO: Update this as needed for HW.
    UART0_LCRL = LCRL_Baud_460800;
    UART0_LCRM = LCRM_Baud_460800;
    UART0_LCRH = LCRH_Word_Length_8;

    // Now enable the serial port
    UART0_CR = CR_UART_Enable | CR_TX_Int_Enable | CR_RX_Int_Enable; // Enable UART0 with no interrupts
}

/**
 * Synchronous API for writing to the UART
 *
 * @param[in] pSrc
 *      Location of the data to send
 *
 * @param[in] Length
 *      Number of bytes to send
 *
 * @retval
 *      None
 */
void UartWrite(void* pSrc, size_t Length)
{
    uint8_t* pBuffer = (uint8_t*)pSrc;

    while (Length-- > 0)
    {
        // wait for open spot
        while (UART0_FR & FR_TX_Fifo_Full)
        {
            // Let others run while waiting
            // tx_thread_relinquish();
        }

        UART0_DR = *pBuffer;
        if (*pBuffer++ == '\n')
        {
            while (UART0_FR & FR_TX_Fifo_Full)
            {
                // Let others run while waiting
                // tx_thread_relinquish();
            }

            UART0_DR = '\r';
        }
    }
}

/**
 * Synchronous API for writing to the UART
 *
 * @param[in] pSrc
 *      Location the data will be placed
 *
 * @param[in] Length
 *      Maximum number of bytes to receive
 *
 * @retval
 *      uint32_t - number of bytes received
 */
uint32_t UartRead(void* pSrc, size_t Length)
{
    uint32_t Count = 0;
    uint8_t* pBuffer = (uint8_t*)pSrc;

    while ((Count < Length) && (!(UART0_FR & FR_RX_Fifo_Empty)))
    {
        Count++;
        *pBuffer++ = UART0_DR;
    }

    return Count;
}
