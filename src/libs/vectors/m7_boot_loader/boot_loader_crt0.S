  .syntax	unified
  .section .init, "ax"
  .code 16
  .align 2
  .thumb_func

 .global _start

// ITCM and DTCM are zero initialized as part of arch_exception_reset handler and hence skipped here
_start:
  CPSID   i
  ldr r1, =__stack_end__
  mov sp, r1

  /* Bootloader data section is in MSCP_EXP RAM, copy it to DTCM. */
  ldr r0, =__data_load_start__
  ldr r1, =__data_start__
  ldr r2, =__data_end__
  bl crt0_memory_copy

  /* Setup call frame for main() */
  mov r0, #0
  mov lr, r0
  mov r12, sp

start:
  /* Jump to main() */
  mov r0, #0
  mov r1, #0
  ldr r2, =main
  blx r2
  /*  when main returns, loop forever. */
crt0_exit_loop:
  wfi
  b crt0_exit_loop

  /* Startup helper functions. */

crt0_memory_copy:
  cmp r0, r1
  beq memory_copy_done
memory_copy_loop:
  ldrb r3, [r0]
  add r0, r0, #1
  strb r3, [r1]
  add r1, r1, #1
  cmp r1, r2
  bne memory_copy_loop
memory_copy_done:
  bx lr