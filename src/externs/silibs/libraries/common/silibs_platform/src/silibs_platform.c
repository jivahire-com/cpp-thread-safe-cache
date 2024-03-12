//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file silibs_platform.c
 * Override for silibs_platform from silibs common.
 * Implements bridging APIs for cortex m7.
 */

/*------------- Includes -----------------*/

#include <silibs_platform.h>
#include <stdio.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void memory_barrier(void)
{
    printf("Calling memory_barrier --> Stub Function not implemented for emCPU (TODO)\n");
}

void memory_barrier_read(void)
{
    printf("Calling memory_barrier_read --> Stub Function not implemented for emCPU (TODO)\n");
}

void memory_barrier_write(void)
{
    printf("Calling memory_barrier_write --> Stub Function not implemented for emCPU (TODO)\n");
}

uint8_t mmio_read8(volatile uint8_t* addr)
{
    return *(volatile uint8_t*)addr;
}

uint16_t mmio_read16(volatile uint16_t* addr)
{
    return *(volatile uint16_t*)addr;
}

uint32_t mmio_read32(volatile uint32_t* addr)
{
    return *(volatile uint32_t*)addr;
}

uint64_t mmio_read64(volatile uint64_t* addr)
{
    return *(volatile uint64_t*)addr;
}

void mmio_write8(volatile uint8_t* addr, uint8_t data)
{
    *(volatile uint8_t*)addr = data;
}

void mmio_write16(volatile uint16_t* addr, uint16_t data)
{
    *(volatile uint16_t*)addr = data;
}

void mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    *(volatile uint32_t*)addr = data;
}

void mmio_write64(volatile uint64_t* addr, uint64_t data)
{
    *(volatile uint64_t*)addr = data;
}

void mmio_update32(volatile uint32_t* addr, uint32_t data, uint32_t mask)
{
    uint32_t val = (mmio_read32(addr) & ~mask) | (data & mask);
    mmio_write32(addr, val);
}

void mmio_update64(volatile uint64_t* addr, uint64_t data, uint64_t mask)
{
    uint64_t val = (mmio_read64(addr) & ~mask) | (data & mask);
    mmio_write64(addr, val);
}

void critical_print(const char* fmt, ...)
{
    printf("critical_print: %s\n", fmt);
}

void info_print(const char* fmt, ...)
{
    printf("info_print: %s\n", fmt);
}

void debug_print(const char* fmt, ...)
{
    printf("debug_print: %s\n", fmt);
}

void sleep_us(uint64_t data)
{
    printf("Calling sleep_us: data=%llu --> Stub Function not implemented for emCPU (TODO)\n", data);
}

void assert_fail_on_line(const char* condition, const char* file, int line)
{
    printf("Calling assert_fail_on_line: condition=%s, file=%s, line=%d --> Stub Function not implemented "
           "for emCPU (TODO)\n",
           condition,
           file,
           line);
}

uint8_t get_platform(void)
{
    return 0;
}

void* platform_memcpy(void* dest, const void* src, size_t count)
{
    // Using standard memcpy. Replace with available cortexm7 specific functions if necessary later.
    memcpy(dest, src, count);
    return 0;
}

void* platform_memset(void* dest, int value, size_t count)
{
    // Using standard memset. Replace with available cortexm7 specific functions if necessary later.
    memset(dest, value, count);
    return 0;
}

bool poll_reg32(uint32_t* addr, uint32_t* rdata, uint32_t masked_match_value, uint32_t mask, uint32_t poll_iterations, uint32_t poll_period_in_us)
{
    bool poll_indefinitely = (poll_iterations == 0);

    do
    {
        *rdata = MMIO_READ32(addr);

        if ((*rdata & mask) == masked_match_value)
        {
            return true;
        }

        if (poll_period_in_us != 0)
        {
            SLEEP_US(poll_period_in_us);
        }
    } while (poll_indefinitely || (--poll_iterations > 0));

    return false;
}
