/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/

/* ThreadX Initialization, Interrupt Handling, and Fault Handling */

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** ThreadX Component                                                     */
/**                                                                       */
/**   Initialize                                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


    .global     _tx_thread_system_stack_ptr
    .global     _tx_initialize_unused_memory
    .global     __RAM_segment_used_end__
    .global     _tx_timer_interrupt
    .global     __main
    .global     _vectors
    .global     __tx_NMIHandler                     // NMI
    .global     __tx_BadHandler                     // HardFault
    .global     __tx_DBGHandler                     // Monitor
    .global     __tx_PendSVHandler                  // PendSV
    .global     __tx_SysTickHandler                 // SysTick
    .global     __tx_IntHandler                     // Int 0

SYSTEM_CLOCK      =   600000000                     // Sys Clock set to Cortex M7 max clock speed of 600MHz
SYSTICK_CYCLES    =   ((SYSTEM_CLOCK / 100) -1)     // SYSTICK set for 100 ticks per second

    .text 32
    .align 4
    .syntax unified
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_initialize_low_level                          Cortex-M7/GNU     */
/*                                                           x.x          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is responsible for any low-level processor            */
/*    initialization, including setting up interrupt vectors, setting     */
/*    up a periodic timer interrupt source, saving the system stack       */
/*    pointer for use in ISR processing later, and finding the first      */
/*    available RAM memory address for tx_application_define.             */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    _tx_initialize_kernel_enter           ThreadX entry function        */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-30-2020     William E. Lamie         Initial Version 6.1           */
/*  xx-xx-xxxx     Scott Larson             Modified comment(s),          */
/*                                            resulting in version x.x    */
/*                                                                        */
/**************************************************************************/
// VOID   _tx_initialize_low_level(VOID)
// {
    .global  _tx_initialize_low_level
    .thumb_func
_tx_initialize_low_level:

    /* Disable interrupts during ThreadX initialization.  */
    
    CPSID   i

    /* Enable divide by zero traps */
    /* TODO : Enable Unaligned access trap ADO 1633029 */

    LDR  r0, =0xE000ED14                            // Build address of CCR
    LDR  r1, [r0]                                   // Pickup the Configuration and Control Register
    LDR  r2, =0x10                                  // Build bits for DIV_0_TRP
    ORR  r1, r1, r2                                 // Set bits for DIV_0_TRP
    STR  r1, [r0]                                   // Set the Configuration and Control Register

    /* Set base of available memory to end of non-initialized RAM area.  */

    LDR     r0, =_tx_initialize_unused_memory       // Build address of unused memory pointer
    LDR     r1, =__RAM_segment_used_end__           // Build first free address
    ADD     r1, r1, #4                              // 
    STR     r1, [r0]                                // Setup first unused memory pointer

    /* Setup Vector Table Offset Register.  */
    
    MOV     r0, #0xE000E000                         // Base address of M7 Config registers
    LDR     r1, =_vectors                           // Pickup address of vector table
    STR     r1, [r0, #0xD08]                        // Set vector table address

    /* Set system stack pointer from vector value.  */

    LDR     r0, =_tx_thread_system_stack_ptr        // Build address of system stack pointer
    LDR     r1, =_vectors                           // Pickup address of vector table
    LDR     r1, [r1]                                // Pickup reset stack pointer
    STR     r1, [r0]                                // Save system stack pointer

    /* Configure SysTick.  */

    MOV     r0, #0xE000E000                         // Base address of M7 Config registers
    LDR     r1, =SYSTICK_CYCLES
    STR     r1, [r0, #0x14]                         // Setup SysTick Reload Value
    MOV     r1, #0x7                                // Build SysTick Control Enable Value
    STR     r1, [r0, #0x10]                         // Setup SysTick Control

    /* Configure handler priorities.  */

    LDR     r1, =0x00000000                         // Rsrv, UsgF, BusF, MemM
    STR     r1, [r0, #0xD18]                        // Setup System Handlers 4-7 Priority Registers

    LDR     r1, =0xFF000000                         // SVCl, Rsrv, Rsrv, Rsrv
    STR     r1, [r0, #0xD1C]                        // Setup System Handlers 8-11 Priority Registers
                                                    // Note: SVC must be lowest priority, which is 0xFF

    LDR     r1, =0x40FF0000                         // SysT, PnSV, Rsrv, DbgM
    STR     r1, [r0, #0xD20]                        // Setup System Handlers 12-15 Priority Registers
                                                    // Note: PnSV must be lowest priority, which is 0xFF

    /* Return to caller.  */

    BX      lr 
// }

/* Generic interrupt handler template */
    .global  __tx_IntHandler
    .thumb_func
__tx_IntHandler:
// VOID InterruptHandler (VOID)
// {
    PUSH    {r0, lr}
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    BL      _tx_execution_isr_enter             // Call the ISR enter function
#endif       

    /* Do interrupt handler work here */
    /* BL <your C Function>.... */

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    BL      _tx_execution_isr_exit              // Call the ISR exit function
#endif
    POP     {r0, lr}
    BX      LR
// }

/* System Tick timer interrupt handler */
    .global  __tx_SysTickHandler
    .global  SysTick_Handler
    .thumb_func
__tx_SysTickHandler:
    .thumb_func
SysTick_Handler:
// VOID TimerInterruptHandler (VOID)
// {

    PUSH    {r0, lr}
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    BL      _tx_execution_isr_enter             // Call the ISR enter function
#endif
    BL      _tx_timer_interrupt
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    BL      _tx_execution_isr_exit              // Call the ISR exit function
#endif
    POP     {r0, lr}
    BX      LR
// }


/* NMI, DBG handlers */
    .global  __tx_NMIHandler 
    .thumb_func
__tx_NMIHandler:
    B       __tx_NMIHandler

    .global  __tx_DBGHandler
    .thumb_func
__tx_DBGHandler:
    B       __tx_DBGHandler

    /* @TODO - Revisit this as to make sure it's still functional now that threadx modules are not used - https://azurecsi.visualstudio.com/DuvallFw/_workitems/edit/1478138 */
    
    /* Memory Exception Handler.  */
    /* BusFault_Handler.          */
    /* UsageFault_Handler.        */
    /* HardFault_Handler.         */
    .global  MemManage_Handler
    .global  BusFault_Handler
    .global  UsageFault_Handler
    .global  HardFault_Handler
    .thumb_func
HardFault_Handler:
    .thumb_func
MemManage_Handler:
    .thumb_func
BusFault_Handler:
    .thumb_func
UsageFault_Handler:

#ifdef TX_PORT_USE_BASEPRI
    LDR     r1, =TX_PORT_BASEPRI                    // Mask interrupt priorities =< TX_PORT_BASEPRI
    MSR     BASEPRI, r1
#else
    CPSID   i                                       // Disable interrupts
#endif  /* TX_PORT_USE_BASEPRI */

    /* Now pickup and store all the fault related information.  */

    LDR     r0, =_tx_thread_current_ptr             // Build current thread pointer address
    LDR     r1, [r0]                                // Pickup the current thread pointer
    STR     r1, [r12, #0]                           // Save current thread pointer in fault info structure
    LDR     r0, =0xE000ED24                         // Build SHCSR address
    LDR     r1, [r0]                                // Pickup SHCSR
    STR     r1, [r12, #8]                           // Save SHCSR
    LDR     r0, =0xE000ED28                         // Build CFSR address
    LDR     r1, [r0]                                // Pickup CFSR
    STR     r1, [r12, #12]                          // Save CFSR
    LDR     r0, =0xE000ED34                         // Build MMFAR address
    LDR     r1, [r0]                                // Pickup MMFAR
    STR     r1, [r12, #16]                          // Save MMFAR
    LDR     r0, =0xE000ED38                         // Build BFAR address
    LDR     r1, [r0]                                // Pickup BFAR
    STR     r1, [r12, #20]                          // Save BFAR
    MRS     r0, CONTROL                             // Pickup current CONTROL register
    STR     r0, [r12, #24]                          // Save CONTROL
    MRS     r1, PSP                                 // Pickup thread stack pointer
    STR     r1, [r12, #28]                          // Save thread stack pointer
    LDR     r0, [r1, #0]                            // Pickup saved r0
    STR     r0, [r12, #32]                          // Save r0
    LDR     r0, [r1, #4]                            // Pickup saved r1
    STR     r0, [r12, #36]                          // Save r1
    LDR     r0, [r1, #8]                            // Pickup saved r2
    STR     r0, [r12, #40]                          // Save r2
    LDR     r0, [r1, #12]                           // Pickup saved r3
    STR     r0, [r12, #44]                          // Save r3
    STR     r4, [r12, #48]                          // Save r4
    STR     r5, [r12, #52]                          // Save r5
    STR     r6, [r12, #56]                          // Save r6
    STR     r7, [r12, #60]                          // Save r7
    STR     r8, [r12, #64]                          // Save r8
    STR     r9, [r12, #68]                          // Save r9
    STR     r10,[r12, #72]                          // Save r10
    STR     r11,[r12, #76]                          // Save r11
    LDR     r0, [r1, #16]                           // Pickup saved r12
    STR     r0, [r12, #80]                          // Save r12
    LDR     r0, [r1, #20]                           // Pickup saved lr
    STR     r0, [r12, #84]                          // Save lr
    LDR     r0, [r1, #24]                           // Pickup instruction address at point of fault
    STR     r0, [r12, #4]                           // Save point of fault
    LDR     r0, [r1, #28]                           // Pickup xPSR
    STR     r0, [r12, #88]                          // Save xPSR
    LDR     r0, =0xE000ED2C                         // Build HFSR address
    LDR     r1, [r0]                                // Pickup HFSR
    STR     r1, [r12, #92]                          // Save HFSR

    MRS     r0, CONTROL                             // Pickup current CONTROL register
    BIC     r0, r0, #1                              // Clear the UNPRIV bit
    MSR     CONTROL, r0                             // Setup new CONTROL register

    LDR     r0, =0xE000ED28                         // Build the Configurable Fault Status Register (CFSR)
    LDR     r1, [r0]                                // Pickup the CFSR
    STR     r1, [r0]                                // Clear the CFSR

    LDR     r0, =0xE000ED2C                         // Build HardFault Status Register (HFSR)
    LDR     r1, [r0]                                // Pickup the HFSR
    STR     r1, [r0]                                // Clear the HFSR

#ifdef __ARM_FP
    LDR     r0, =0xE000EF34                         // Cleanup FPU context: Load FPCCR address
    LDR     r1, [r0]                                // Load FPCCR
    BIC     r1, r1, #1                              // Clear the lazy preservation active bit
    STR     r1, [r0]                                // Save FPCCR
#endif

#if (defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE))
    /* Call the thread exit function to indicate the thread is no longer executing.  */
    CPSID   i                                       // Disable interrupts
    BL      _tx_execution_thread_exit               // Call the thread exit function
    CPSIE   i                                       // Enable interrupts
#endif

    MOV     r1, #0                                  // Build NULL value
    LDR     r0, =_tx_thread_current_ptr             // Pickup address of current thread pointer
    STR     r1, [r0]                                // Clear current thread pointer

    // Return from MemManage_Handler/BusFault_Handler/UsageFault_Handler exception

    LDR     r0, =0xE000ED04                         // Load ICSR
    LDR     r1, =0x10000000                         // Set PENDSVSET bit
    STR     r1, [r0]                                // Store ICSR
    DSB                                             // Wait for memory access to complete

#ifdef TX_PORT_USE_BASEPRI
    MOV     r0, 0                                   // Disable BASEPRI masking (enable interrupts)
    MSR     BASEPRI, r0
#else
    CPSIE   i                                       // Enable interrupts
#endif
    MOV     lr, #0xFFFFFFFD                         // Load exception return code
    BX      lr                                      // Return from exception
