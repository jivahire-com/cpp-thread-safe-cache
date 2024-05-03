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
  b crt0_exit_loop
