//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file cortex_m7.c
 *  This modules provides cortex m7 functionality on-top of whats provided by threadx.
 */

/*------------- Includes -----------------*/

#include <cortex_m7.h>
#include <stdbool.h>
#include <stdint.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// clang-format off

uint32_t cortex_m7_atomic_increment(volatile uint32_t* address, uint32_t value)
{
    uint32_t new_val = 0;
    __asm__ volatile("    dsb \n"                                // Data Synchronization Barrier
                     "ainc_start: \n"                            // Loop Start
                     "    ldrex r2, [%[address]] \n"             // Load r2 with value at address
                     "    add   %[new_val], r2, %[value] \n"     // Increment r2 by value
                     "    strex r3, %[new_val], [%[address]] \n" // Store new val back into the address
                     "    cbz   r3, ainc_end \n"                 // Success? Branch to loop end
                     "    b     ainc_start \n"                   // Failure? Branch to loop start
                     "ainc_end: \n"                              // Loop End
                     "    dsb \n"                                // Data Synchronization Barrier
                     : [new_val] "=&r"(new_val)                  // Outputs
                     : [address] "r"(address),                   // Inputs
                       [value] "r"(value)
                     : "r2", "r3", "cc", "memory");              // Clobbers

    return new_val;
}

uint32_t cortex_m7_atomic_decrement(volatile uint32_t* address, uint32_t value)
{
    uint32_t new_val = 0;
    __asm__ volatile("    dsb \n"                                // Data Synchronization Barrier
                     "adec_start: \n"                            // Loop Start
                     "    ldrex r2, [%[address]] \n"             // Load r2 with value at address
                     "    sub   %[new_val], r2, %[value] \n"     // Increment r2 by value
                     "    strex r3, %[new_val], [%[address]] \n" // Store r2 back into the address
                     "    cbz   r3, adec_end \n"                 // Success? Branch to loop end
                     "    b     adec_start \n"                   // Failure? Branch to loop start
                     "adec_end: \n"                              // Loop End
                     "    dsb \n"                                // Data Synchronization Barrier
                     : [new_val] "=&r"(new_val)                  // Outputs
                     : [address] "r"(address),                   // Inputs
                       [value] "r"(value)
                     : "r2", "r3", "cc", "memory");              // Clobbers

    return new_val;
}

uint32_t cortex_m7_atomic_compare_exchange(volatile uint32_t* address, uint32_t value, uint32_t compare)
{
    uint32_t current_val = 0;
    __asm__ volatile("    dsb \n"                                   // Data Synchronization Barrier
                    "cmpex_start: \n"                               // Loop Start
                    "    ldrex %[current_val], [%[address]] \n"     // Load r2 with value at address
                    "    cmp   %[current_val], %[compare] \n"       // Compare r2 with the comparison value
                    "    beq   cmpex_eq \n"                         // Branch if equal
                    "    strex r3, %[current_val], [%[address]] \n" // Not Equal, store the existing value back
                    "    cbz   r3, cmpex_end \n"                    // Success? Branch to loop end
                    "    b     cmpex_start \n"                      // Failure? Branch to loop start
                    "cmpex_eq: \n"                                  // 
                    "    strex r3, %[value], [%[address]] \n"       // Equal, store the new value
                    "    cbz   r3, cmpex_end \n"                    // Success? Branch to loop end
                    "    b     cmpex_start \n"                      // Failure? Branch to loop start
                    "cmpex_end: \n"                                 // Loop End
                    "    dsb \n"                                    // Data Synchronization Barrier
                    : [current_val] "=&r"(current_val)              // Outputs
                    : [address] "r"(address),                       // Inputs
                      [compare] "r"(compare),
                      [value] "r"(value)
                    : "r3", "cc", "memory");                        // Clobbers

    return current_val;
}

// clang-format on
