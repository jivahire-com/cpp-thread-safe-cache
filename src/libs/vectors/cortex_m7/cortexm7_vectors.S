//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
  .global reset_handler

  .global __tx_NMIHandler
  .global __tx_DefaultHandler
  .global __tx_SVCallHandler
  .global __tx_DBGHandler
  .global __tx_PendSVHandler 
  .global __tx_SysTickHandler 
  
  .syntax unified
  .section .vectors, "ax"
  .code 16
  .align 0
  .global _vectors

_vectors:
  .word __stack_end__ 
  .word reset_handler 
  .word __tx_NMIHandler 
  .word HardFault_Handler
  .word MemManage_Handler
  .word BusFault_Handler
  .word UsageFault_Handler
  .word 0 // Reserved
  .word 0 // Reserved
  .word 0 // Reserved
  .word 0 // Reserved
  .word __tx_SVCallHandler    //_SVC_Handler - used by Threadx scheduler //
  .word __tx_DBGHandler
  .word 0 // Reserved
  .word __tx_PendSVHandler 
  .word __tx_SysTickHandler   // Used by Threadx timer functionality
  .word __tx_DefaultHandler   // Populate with user Interrupt handler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler
  .word __tx_DefaultHandler

  .section .init, "ax"
  .thumb_func
reset_handler:
  // low level hardware config, such as PLL setup goes here
  b _start


/* Define shells for each of the unused vectors.  */

    .global  __tx_DefaultHandler
    .thumb_func
__tx_DefaultHandler:
    MRS  r0, IPSR        // Get the Interrupt Program Status Register
    LDR  r1, =0xE000ED28 // Get address of CFSR
    LDR  r1, [r1]        // Get the Configurable Fault Status Register
    LDR  r2, =0xE000ED34 // Get address of MMFAR
    LDR  r2, [r2]        // Get the MemManage Fault Address Register
    LDR  r3, =0xE000ED38 // Get address of BFAR
    LDR  r3, [r3]        // Get the Bus Fault Address Register

    MRS  r12, PSP        // Get the PSP stack pointer

deadloop:    
    B       deadloop
