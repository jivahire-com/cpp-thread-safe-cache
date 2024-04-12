//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file Debug.c
 * This file contains the implementation for the Debug library
 */

// @TODO - Implement this library, including the design document for it.

/*------------- Includes -----------------*/
#include "debug.h"

#include "bugcheck.h"
#include "uart.h"

#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

// Tell cspell to ignore the "reent" parameter
/* cSpell:ignore reent */

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
__asm(".global __use_no_semihosting");
FILE __stdout; // NOLINT
FILE __stdin;  // NOLINT
FILE __stderr; // NOLINT

/*-------------- Functions ---------------*/
void debug_init()
{
}

char* number_to_string(unsigned int number)
{
    static char buf[11];
    char* ptr = &buf[10];
    *ptr = '\0';
    do
    {
        ptr--;

        int digit = number % 10;
        number = number / 10;

        *ptr = (char)(digit + '0');
    } while (number != 0);

    return ptr;
}

char* tx_str(int result)
{
    switch (result)
    {
    case 0x00:
        return "TX_SUCCESS";
    case 0x01:
        return "TX_DELETED";
    case 0x02:
        return "TX_POOL_ERROR";
    case 0x03:
        return "TX_PTR_ERROR";
    case 0x04:
        return "TX_WAIT_ERROR";
    case 0x05:
        return "TX_SIZE_ERROR";
    case 0x06:
        return "TX_GROUP_ERROR";
    case 0x07:
        return "TX_NO_EVENTS";
    case 0x08:
        return "TX_OPTION_ERROR";
    case 0x09:
        return "TX_QUEUE_ERROR";
    case 0x0A:
        return "TX_QUEUE_EMPTY";
    case 0x0B:
        return "TX_QUEUE_FULL";
    case 0x0C:
        return "TX_SEMAPHORE_ERROR";
    case 0x0D:
        return "TX_NO_INSTANCE";
    case 0x0E:
        return "TX_THREAD_ERROR";
    case 0x0F:
        return "TX_PRIORITY_ERROR";
    case 0x10:
        return "TX_NO_MEMORY/TX_START_ERROR";
    case 0x11:
        return "TX_DELETE_ERROR";
    case 0x12:
        return "TX_RESUME_ERROR";
    case 0x13:
        return "TX_CALLER_ERROR";
    case 0x14:
        return "TX_SUSPEND_ERROR";
    case 0x15:
        return "TX_TIMER_ERROR";
    case 0x16:
        return "TX_TICK_ERROR";
    case 0x17:
        return "TX_ACTIVATE_ERROR";
    case 0x18:
        return "TX_THRESH_ERROR";
    case 0x19:
        return "TX_SUSPEND_LIFTED";
    case 0x1A:
        return "TX_WAIT_ABORTED";
    case 0x1B:
        return "TX_WAIT_ABORT_ERROR";
    case 0x1C:
        return "TX_MUTEX_ERROR";
    case 0x1D:
        return "TX_NOT_AVAILABLE";
    case 0x1E:
        return "TX_NOT_OWNED";
    case 0x1F:
        return "TX_INHERIT_ERROR";
    case 0x20:
        return "TX_NOT_DONE";
    case 0x21:
        return "TX_CEILING_EXCEEDED";
    case 0x22:
        return "TX_INVALID_CEILING";

    case 0xF0:
        return "TXM_MODULE_ALIGNMENT_ERROR";
    case 0xF1:
        return "TXM_MODULE_ALREADY_LOADED";
    case 0xF2:
        return "TXM_MODULE_INVALID";
    case 0xF3:
        return "TXM_MODULE_INVALID_PROPERTIES";
    case 0xF4:
        return "TXM_MODULE_INVALID_MEMORY";
    case 0xF5:
        return "TXM_MODULE_INVALID_CALLBACK";
    case 0xF6:
        return "TXM_MODULE_INVALID_STACK_SIZE";
    case 0xF7:
        return "TXM_MODULE_FILEX_INVALID_BYTES_READ";
    case 0xF8:
        return "TXM_MODULE_MATH_OVERFLOW";

    case 0xFF:
        return "TX_FEATURE_NOT_ENABLED";
    default:
        return "unknown";
    }
}

void _bugcheck(uint32_t bugcheck_code, uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    printf("BUGCHECK(%08" PRIx32 ", %08" PRIx32 ", %08" PRIx32 ", %08" PRIx32 ")\n", bugcheck_code, arg0, arg1, arg2);

    switch (bugcheck_code)
    {
    case BUGCHECK_FAILFAST:
        printf("Failfast in %s:%s\n", (char*)arg2, number_to_string(arg1));
        break;
    case BUGCHECK_ASSERT:
        printf("Assertion false in %s:%s\n", (char*)arg2, number_to_string(arg1));
        break;
    case BUGCHECK_ASSERT_EQ:
    case BUGCHECK_ASSERT_NEQ:
        printf("Assertion failed in %s:%s\n", (char*)arg2, number_to_string(arg1));
        printf("Actual value is \"0x%08" PRIx32 "\".\n", arg0);
        break;
    case BUGCHECK_ASSERT_TX_SUCCESS:
        printf("Assertion failed in %s:%s\n", (char*)arg2, number_to_string(arg1));
        printf("Result is \"0x%02" PRIx32 "\": %s\n", arg0, tx_str(arg0));
        break;
    }
    (void)bugcheck_code;
    (void)arg0;
    (void)arg1;
    (void)arg2;
    while (1)
    {
        // @TODO - Implement with bugcheck / crashdump handling
    }
}

/*******************************
 * Remapping IO functions
 *
 *******************************/

int _write_r(struct _reent* reent, int fh, const unsigned char* buf, unsigned len)
{
    UNUSED(reent);
    UNUSED(fh);

    UartWrite((void*)buf, len);
    return len;
}

int _read_r(struct _reent* reent, int fh, unsigned char* buf, unsigned len)
{
    UNUSED(reent);
    UNUSED(fh);

    return UartRead(buf, len);
}

int _open_r(struct _reent* reent, const char* name, int openmode, int flags)
{
    int fh = 0;
    UNUSED(reent);
    UNUSED(name);
    UNUSED(openmode);
    UNUSED(flags);

    return fh;
}

int _close_r(struct _reent* reent, int fh)
{
    UNUSED(reent);
    UNUSED(fh);

    return 0;
}

int _isatty_r(struct _reent* reent, int fh)
{
    UNUSED(reent);
    UNUSED(fh);

    return 0;
}

int _lseek_r(struct _reent* reent, int fh, long pos)
{
    UNUSED(reent);
    UNUSED(fh);
    UNUSED(pos);

    return -1;
}

void _exit(int __status)
{
    UNUSED(__status);

    while (1)
    {
        ;
    }
}

// Implement _sbrk_r to enable the heap for malloc
void* _sbrk_r(struct _reent* reent, int inc)
{
    UNUSED(reent);
    // These symbols are defined by the linker and determine the
    // maximum space available for the heap
    extern char end asm("end"); // the "end" of the .bss section. The c runtime assumes the heap begins here
    extern char heap_section_end asm("heap_end"); // defined by linker

    // heap_runtime_end is the c runtime's understanding of where the end of the heap currently is
    static char* heap_runtime_end;
    char* prev_heap_runtime_end;

    if (heap_runtime_end == NULL)
    {
        // initially, the c runtime assumes there is zero space for the heap, and so
        // heap_end should be initialized to heap_section_start (which is the same as .bss end)
        heap_runtime_end = &end;
    }

    prev_heap_runtime_end = heap_runtime_end;

    if (heap_runtime_end + inc > &heap_section_end)
    {
        // We cannot grow the heap any further
        return (void*)-1;
    }

    heap_runtime_end += inc;

    // On success we return the previous heap end position
    return prev_heap_runtime_end;
}

int _fstat_r(struct _reent* reent, int fd, struct stat* buf)
{
    UNUSED(reent);
    UNUSED(fd);
    UNUSED(buf);

    return -1;
}

int _kill_r(struct _reent* reent, pid_t pid, int sig)
{
    UNUSED(reent);
    UNUSED(pid);
    UNUSED(sig);

    return -1;
}

pid_t _getpid_r(struct _reent* reent)
{
    UNUSED(reent);

    return 0;
}