  .global reset_handler

  .syntax unified
  .section .vectors, "ax"
  .code 16
  .align 0
  .global _vectors

// All vectors except for reset vector will invoke a default looping handler
_vectors:
  .word __stack_end__ 
  .word reset_handler 
  .word __default_exception_handler 
  .word __default_exception_handler
  .word __default_exception_handler
  .word __default_exception_handler
  .word __default_exception_handler
  .word 0 // Reserved
  .word 0 // Reserved
  .word 0 // Reserved
  .word 0 // Reserved
  .word __default_exception_handler    
  .word __default_exception_handler
  .word 0 // Reserved


  .section .init, "ax"
  .thumb_func
reset_handler:
  // Reset ITCM, DTCM to zero and jump to _start
  //The arch_exception_reset pushes r3, lr to stack hence updating SP here
  ldr r1, =__stack_end__
  mov sp, r1
  ldr r0, =arch_exception_reset
  bx r0

/* Define shells for each of the unused vectors.  */
//TODO: Need to add logic to generate a watchdog if needed
//ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1728673

    .global  __default_exception_handler
    .thumb_func
__default_exception_handler:
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