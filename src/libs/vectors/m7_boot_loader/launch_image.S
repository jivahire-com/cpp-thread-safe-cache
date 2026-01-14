//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/*
 * Arm SCP/MCP Software
 * Copyright (c) 2020-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

    .syntax unified

    .text

/*
 * noreturn int launch_image(
 *     void *destination,
 *     uint32_t *vtor);
 */
    .thumb
    .thumb_func
    .global launch_image
    .type launch_image, %function

launch_image:
    
    str r0, [r1] /* Store vector table address in SCB->VTOR (if it exists) */
    dsb
    ldr r4, [r0] /* Grab new stack pointer from vector table... */
    msr msp, r4 /* ... and update the main stack pointer with it */
    isb /* Ensure stack pointer is updated before proceeding */

    ldr r2, [r0, #4] /* Load the reset address from the vector table... */
    bx r2 /* ... and take a leap of faith */

    udf /* the branch should never return. this memory will be re-used for other purposes, but if
           the call were to return before it is cleared, we will force a crash on this undefined
           instruction */

never_return:
    b never_return
    .pool
